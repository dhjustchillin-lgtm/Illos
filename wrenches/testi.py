import argparse
import glob
import io
import json
import os
import re
import struct
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    from PIL import Image, ImageOps
except ImportError:
    print("[CRITICAL] 'Pillow' library is required. Run: pip install Pillow")
    sys.exit(1)

# Layer type options
LAYER_TYPES = {
    0: "Normal (Middle & Top)",
    1: "Covered (Bottom & Middle)",
    2: "Split (Bottom & Top)",
}

# Terrain Types (32-bit mode)
TERRAIN_TYPES = {
    0: "Normal",
    1: "Grass",
    2: "Water",
    3: "Waterfall",
    4: "Sand",
    5: "Ice",
}

# Encounter Types (32-bit mode)
ENCOUNTER_TYPES = {
    0: "None",
    1: "Land / Grass",
    2: "Water",
}

STUDIO = {
    "dir_path": "",
    "tiles_png_path": "",
    "metatiles_bin_path": "",
    "attr_bin_path": "",
    "palettes": {},  # Map of pal_idx (0-15) -> list of 16 RGB tuples
    "tiles_img": None,  # Raw 8x8 indexed tiles image
    "metatiles": [],  # List of 8-u16 entries per metatile
    "metatile_attrs": [],  # Parsed attributes per metatile
    "attr_bytes": 2,  # 2 bytes per attribute (Standard Emerald) or 4 bytes
    "behavior_map": {},  # Map of behavior_id -> behavior_name string
    "primary_tile_count": 512,  # Standard offset for secondary tilesets
}


def load_behavior_constants(header_path: str) -> dict:
    """Parses metatile_behaviors.h for enums and defines."""
    behaviors = {}
    if not os.path.exists(header_path):
        print(
            f"[WARNING] Header file '{header_path}' not found. Behavior names won't be resolved."
        )
        return behaviors

    with open(header_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    enum_matches = re.findall(
        r"enum\s*\{([^}]+)\}", content, re.MULTILINE | re.DOTALL
    )
    for enum_body in enum_matches:
        current_val = 0
        enum_body = re.sub(r"//.*|/\*[\s\S]*?\*/", "", enum_body)

        for item in enum_body.split(","):
            item = item.strip()
            if not item:
                continue

            if "=" in item:
                name, val_str = item.split("=", 1)
                name = name.strip()
                val_str = val_str.strip()
                current_val = (
                    int(val_str, 16)
                    if val_str.startswith("0x")
                    else int(val_str)
                )
            else:
                name = item.strip()

            behaviors[current_val] = name
            current_val += 1

    define_pattern = re.compile(
        r"#define\s+(MB_\w+)\s+(0x[0-9A-Fa-f]+|\d+|UCHAR_MAX)"
    )
    for match in define_pattern.finditer(content):
        name, val_str = match.groups()
        val = (
            255
            if val_str == "UCHAR_MAX"
            else (
                int(val_str, 16)
                if val_str.startswith("0x")
                else int(val_str)
            )
        )
        behaviors[val] = name

    return behaviors


def load_jasc_pal(filepath):
    colors = []
    if not os.path.exists(filepath):
        return None
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            lines = [line.strip() for line in f if line.strip()]
            if len(lines) >= 3 and lines[0] == "JASC-PAL":
                for line in lines[3:]:
                    parts = line.split()
                    if len(parts) >= 3:
                        colors.append(
                            (int(parts[0]), int(parts[1]), int(parts[2]))
                        )
            else:
                for line in lines:
                    match = re.match(r"^(\d+)\s+(\d+)\s+(\d+)$", line)
                    if match:
                        colors.append(
                            (
                                int(match.group(1)),
                                int(match.group(2)),
                                int(match.group(3)),
                            )
                        )
    except Exception:
        pass

    while len(colors) < 16:
        colors.append((0, 0, 0))
    return colors[:16]


def load_all_palettes(dir_path):
    palettes = {}
    pal_dir = os.path.join(dir_path, "palettes")
    if not os.path.exists(pal_dir):
        pal_dir = dir_path

    for i in range(16):
        pal_file = os.path.join(pal_dir, f"{i:02d}.pal")
        if os.path.exists(pal_file):
            cols = load_jasc_pal(pal_file)
            if cols:
                palettes[i] = cols

    if not palettes:
        default_pal = [(0, 0, 0)] + [
            (i * 16, i * 16, i * 16) for i in range(1, 16)
        ]
        for i in range(16):
            palettes[i] = list(default_pal)

    return palettes


def stage_tileset(
    dir_path, header_path, attr_bytes, primary_tile_count=512
):
    STUDIO["dir_path"] = dir_path
    STUDIO["attr_bytes"] = attr_bytes
    STUDIO["behavior_map"] = load_behavior_constants(header_path)
    STUDIO["primary_tile_count"] = primary_tile_count

    png_files = glob.glob(os.path.join(dir_path, "*.png"))
    STUDIO["tiles_png_path"] = (
        png_files[0] if png_files else os.path.join(dir_path, "tiles.png")
    )
    STUDIO["metatiles_bin_path"] = os.path.join(dir_path, "metatiles.bin")
    STUDIO["attr_bin_path"] = os.path.join(dir_path, "metatile_attributes.bin")

    STUDIO["palettes"] = load_all_palettes(dir_path)

    if os.path.exists(STUDIO["tiles_png_path"]):
        raw_img = Image.open(STUDIO["tiles_png_path"])
        if raw_img.mode != "P":
            STUDIO["tiles_img"] = raw_img.convert(
                "P", palette=Image.Palette.ADAPTIVE, colors=256
            )
        else:
            STUDIO["tiles_img"] = raw_img
    else:
        STUDIO["tiles_img"] = Image.new("P", (128, 128), 0)

    # Load Metatiles with strict 16-byte alignment (8 uint16 per metatile)
    STUDIO["metatiles"] = []
    if os.path.exists(STUDIO["metatiles_bin_path"]):
        with open(STUDIO["metatiles_bin_path"], "rb") as f:
            data = f.read()
            total_metatiles = len(data) // 16
            for i in range(total_metatiles):
                offset = i * 16
                entries = list(struct.unpack_from("<8H", data, offset))
                STUDIO["metatiles"].append(entries)

    # Load Metatile Attributes
    STUDIO["metatile_attrs"] = []
    if os.path.exists(STUDIO["attr_bin_path"]):
        with open(STUDIO["attr_bin_path"], "rb") as f:
            data = f.read()
            file_size = len(data)

            if (
                attr_bytes == 2
                and file_size % 4 == 0
                and (file_size // 4) == len(STUDIO["metatiles"])
            ):
                STUDIO["attr_bytes"] = 4
            elif (
                attr_bytes == 4
                and file_size % 2 == 0
                and (file_size // 2) == len(STUDIO["metatiles"])
            ):
                STUDIO["attr_bytes"] = 2

            entry_bytes = STUDIO["attr_bytes"]
            num_entries = file_size // entry_bytes

            for i in range(num_entries):
                offset = i * entry_bytes
                if entry_bytes == 2:
                    val = struct.unpack_from("<H", data, offset)[0]
                    STUDIO["metatile_attrs"].append(
                        {
                            "behavior": val & 0x01FF,
                            "layer": (val & 0xF000) >> 12,
                            "terrain": 0,
                            "encounter": 0,
                        }
                    )
                else:
                    val = struct.unpack_from("<I", data, offset)[0]
                    STUDIO["metatile_attrs"].append(
                        {
                            "behavior": val & 0x000001FF,
                            "terrain": (val & 0x00003E00) >> 9,
                            "encounter": (val & 0x07000000) >> 24,
                            "layer": (val & 0x60000000) >> 29,
                        }
                    )

    while len(STUDIO["metatile_attrs"]) < len(STUDIO["metatiles"]):
        STUDIO["metatile_attrs"].append(
            {"behavior": 0, "layer": 0, "terrain": 0, "encounter": 0}
        )


def render_8x8_tile(tile_u16):
    """Parses individual 8x8 sub-tile and decodes bitfield specs correctly."""
    tile_id = tile_u16 & 0x03FF
    x_flip = bool((tile_u16 >> 10) & 0x01)
    y_flip = bool((tile_u16 >> 11) & 0x01)
    pal_idx = (tile_u16 >> 12) & 0x0F

    tiles_img = STUDIO["tiles_img"]
    tiles_per_row = max(1, tiles_img.width // 8)
    total_tiles_in_png = tiles_per_row * (tiles_img.height // 8)

    # Offset subtraction for secondary tileset ranges
    if tile_id >= total_tiles_in_png and tile_id >= STUDIO["primary_tile_count"]:
        tile_id -= STUDIO["primary_tile_count"]

    col = tile_id % tiles_per_row
    row = tile_id // tiles_per_row

    crop_box = (col * 8, row * 8, (col + 1) * 8, (row + 1) * 8)

    if (
        crop_box[2] <= tiles_img.width
        and crop_box[3] <= tiles_img.height
        and row >= 0
    ):
        tile_crop = tiles_img.crop(crop_box)
    else:
        tile_crop = Image.new("P", (8, 8), 0)

    pal_colors = STUDIO["palettes"].get(
        pal_idx, STUDIO["palettes"].get(0, [(0, 0, 0)] * 16)
    )

    tile_rgba = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
    pixels = list(tile_crop.getdata())
    final_pixels = []

    for pixel_val in pixels:
        color_idx = pixel_val % 16
        if color_idx == 0:
            final_pixels.append((0, 0, 0, 0))
        else:
            r, g, b = pal_colors[color_idx]
            final_pixels.append((r, g, b, 255))

    tile_rgba.putdata(final_pixels)

    if x_flip:
        tile_rgba = ImageOps.mirror(tile_rgba)
    if y_flip:
        tile_rgba = ImageOps.flip(tile_rgba)

    return tile_rgba


def render_metatile(metatile_entry):
    """Composites 16x16 metatiles matching standard GBA lower/upper layer ordering."""
    img = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    positions = [(0, 0), (8, 0), (0, 8), (8, 8)]

    # Layer 0 (Bottom Layer)
    for idx in range(4):
        tile_u16 = metatile_entry[idx]
        sub_tile = render_8x8_tile(tile_u16)
        img.paste(sub_tile, positions[idx], sub_tile)

    # Layer 1 (Top Layer)
    for idx in range(4):
        tile_u16 = metatile_entry[4 + idx]
        if (tile_u16 & 0x03FF) != 0:
            sub_tile = render_8x8_tile(tile_u16)
            img.paste(sub_tile, positions[idx], sub_tile)

    return img


def save_metatiles_to_disk():
    try:
        u16_list = []
        for meta in STUDIO["metatiles"]:
            u16_list.extend(meta[:8])

        with open(STUDIO["metatiles_bin_path"], "wb") as f:
            f.write(struct.pack(f"<{len(u16_list)}H", *u16_list))

        if STUDIO["metatile_attrs"]:
            entry_bytes = STUDIO["attr_bytes"]

            with open(STUDIO["attr_bin_path"], "wb") as f:
                for attr in STUDIO["metatile_attrs"]:
                    behavior = attr.get("behavior", 0)
                    layer = attr.get("layer", 0)

                    if entry_bytes == 2:
                        val = (behavior & 0x01FF) | ((layer & 0x000F) << 12)
                        f.write(struct.pack("<H", val))
                    else:
                        terrain = attr.get("terrain", 0)
                        encounter = attr.get("encounter", 0)

                        val = (
                            (behavior & 0x000001FF)
                            | ((terrain & 0x0000001F) << 9)
                            | ((encounter & 0x00000007) << 24)
                            | ((layer & 0x00000003) << 29)
                        )
                        f.write(struct.pack("<I", val))

        print(
            f"[SUCCESS] Saved metatiles & attributes to disk ({STUDIO['attr_bytes'] * 8}-bit mode)."
        )
        return True
    except Exception as e:
        print(f"[ERROR] Failed to save metatiles: {e}")
        return False


class TilesetWebBackend(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()

            total_raw_tiles = 0
            if STUDIO["tiles_img"]:
                total_raw_tiles = (STUDIO["tiles_img"].width // 8) * (
                    STUDIO["tiles_img"].height // 8
                )

            html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Pokeemerald Metatile Studio</title>
    <style>
        body {{ margin: 0; font-family: 'Segoe UI', sans-serif; background: #1e1e1e; color: #d4d4d4; display: flex; height: 100vh; overflow: hidden; }}
        #sidebar {{ width: 440px; background: #252526; border-right: 1px solid #3c3c3c; display: flex; flex-direction: column; padding: 15px; box-sizing: border-box; overflow-y: auto; }}
        #main {{ flex: 1; display: flex; flex-direction: column; background: #1e1e1e; }}
        #toolbar {{ height: 50px; background: #2d2d2d; border-bottom: 1px solid #3c3c3c; display: flex; align-items: center; padding: 0 20px; gap: 10px; }}
        #content {{ flex: 1; overflow: auto; padding: 20px; display: flex; justify-content: center; align-items: flex-start; }}
        .grid-container {{ display: grid; grid-template-columns: repeat(8, 64px); gap: 4px; background: #111; padding: 10px; border-radius: 6px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }}
        .metatile-card {{ width: 64px; height: 64px; background: #2a2a2a; border: 2px solid #3c3c3c; border-radius: 4px; cursor: pointer; position: relative; image-rendering: pixelated; }}
        .metatile-card:hover {{ border-color: #0e639c; }}
        .metatile-card.selected {{ border-color: #007acc; box-shadow: 0 0 8px #007acc; }}
        .metatile-card img {{ width: 100%; height: 100%; image-rendering: pixelated; }}
        .metatile-id {{ position: absolute; bottom: 2px; right: 2px; background: rgba(0,0,0,0.7); font-size: 10px; padding: 1px 3px; border-radius: 2px; }}
        button {{ background: #0e639c; color: white; border: none; padding: 6px 12px; border-radius: 4px; cursor: pointer; font-weight: bold; font-size: 11px; }}
        button:hover {{ background: #1177bb; }}
        button.btn-danger {{ background: #d9534f; }}
        button.btn-danger:hover {{ background: #c9302c; }}
        .sidebar-section {{ margin-bottom: 15px; }}
        .sidebar-title {{ font-size: 12px; font-weight: bold; color: #888; text-transform: uppercase; margin-bottom: 8px; border-bottom: 1px solid #333; padding-bottom: 4px; }}
        .preview-box {{ width: 128px; height: 128px; border: 2px solid #3c3c3c; background: #000; margin: 0 auto 10px auto; image-rendering: pixelated; }}
        .preview-box img {{ width: 100%; height: 100%; image-rendering: pixelated; }}
        .attr-box {{ background: #1e1e1e; border: 1px solid #3c3c3c; border-radius: 4px; padding: 8px; display: flex; flex-direction: column; gap: 6px; }}
        .attr-row {{ display: flex; align-items: center; justify-content: space-between; font-size: 11px; }}
        .attr-row label {{ color: #aaa; width: 110px; }}
        .attr-row select {{ flex: 1; background: #111; border: 1px solid #444; color: #fff; font-size: 11px; padding: 3px; border-radius: 2px; }}
        .chunks-grid {{ display: grid; grid-template-columns: 1fr 1fr; gap: 6px; }}
        .chunk-card {{ background: #1e1e1e; border: 1px solid #3c3c3c; border-radius: 4px; padding: 6px; display: flex; flex-direction: column; gap: 4px; cursor: pointer; }}
        .chunk-card.selected {{ border-color: #007acc; background: #2d2d30; }}
        .chunk-header {{ display: flex; align-items: center; justify-content: space-between; font-size: 11px; font-weight: bold; color: #ccc; }}
        .chunk-body {{ display: flex; gap: 8px; align-items: center; }}
        .chunk-preview {{ width: 32px; height: 32px; border: 1px solid #444; background: #000; image-rendering: pixelated; flex-shrink: 0; }}
        .chunk-controls {{ display: flex; flex-direction: column; gap: 3px; flex: 1; }}
        .chunk-controls label {{ font-size: 10px; color: #aaa; width: 35px; }}
        .chunk-controls input[type="number"], .chunk-controls select {{ background: #111; border: 1px solid #444; color: #fff; font-size: 11px; padding: 2px 4px; border-radius: 2px; width: 100%; box-sizing: border-box; }}
        .btn-toggle {{ padding: 2px 5px; font-size: 10px; background: #333; border: 1px solid #555; color: #ccc; }}
        .btn-toggle.on {{ background: #0e639c; color: #fff; border-color: #007acc; }}
        .raw-atlas-wrapper {{ overflow-x: auto; max-height: 180px; overflow-y: auto; background: #111; padding: 4px; border: 1px solid #333; border-radius: 4px; }}
        .raw-atlas-grid {{ display: grid; grid-template-columns: repeat(16, 22px); gap: 2px; width: max-content; }}
        .raw-tile-cell {{ width: 22px; height: 22px; background: #222; border: 1px solid #333; cursor: pointer; box-sizing: border-box; image-rendering: pixelated; }}
        .raw-tile-cell:hover {{ border-color: #007acc; }}
        .save-btn {{ width: 100%; padding: 10px; margin-top: 10px; background: #28a745; color: #fff; font-size: 12px; font-weight: bold; border-radius: 4px; border: none; cursor: pointer; }}
        .save-btn:hover {{ background: #218838; }}
    </style>
</head>
<body>
    <div id="sidebar">
        <div class="sidebar-section">
            <div class="sidebar-title">Selected Metatile</div>
            <div class="preview-box"><img id="selected-img" src="" alt="Select Metatile"></div>
            <div id="selected-info" style="text-align: center; font-weight: bold; margin-bottom: 10px;">Select a Metatile</div>
        </div>
        <div class="sidebar-section">
            <div class="sidebar-title">Metatile Attributes</div>
            <div class="attr-box">
                <div class="attr-row"><label>Layer Type:</label><select id="attr-layer" onchange="updateAttrProperty('layer', this.value)"></select></div>
                <div class="attr-row"><label>Metatile Behavior:</label><select id="attr-behavior" onchange="updateAttrProperty('behavior', this.value)"></select></div>
                <div class="attr-row" id="row-terrain" style="display: none;"><label>Terrain Type:</label><select id="attr-terrain" onchange="updateAttrProperty('terrain', this.value)"></select></div>
                <div class="attr-row" id="row-encounter" style="display: none;"><label>Encounter Type:</label><select id="attr-encounter" onchange="updateAttrProperty('encounter', this.value)"></select></div>
            </div>
        </div>
        <div class="sidebar-section">
            <div class="sidebar-title">Raw Tile Atlas</div>
            <div class="raw-atlas-wrapper"><div class="raw-atlas-grid" id="raw-atlas"></div></div>
        </div>
        <div class="sidebar-section">
            <div class="sidebar-title">8x8 Sub-Tile Breakdown</div>
            <div class="chunks-grid" id="chunks-container"></div>
            <button class="save-btn" onclick="saveMetatiles()">💾 SAVE CHANGES TO DISK</button>
        </div>
    </div>
    <div id="main">
        <div id="toolbar">
            <span style="font-weight: bold;">Tileset Target:</span>
            <span>{os.path.basename(STUDIO["dir_path"])}</span>
            <span style="margin-left: 20px; color: #888;">Mode: {STUDIO['attr_bytes'] * 8}-bit Attributes</span>
            <span style="margin-left: auto; color: #aaa;">Total Metatiles: <span id="metatile-count">{len(STUDIO["metatiles"])}</span></span>
            <button onclick="addMetatile()">➕ ADD METATILE</button>
            <button class="btn-danger" onclick="deleteMetatile()">🗑️ DELETE METATILE</button>
        </div>
        <div id="content"><div class="grid-container" id="grid"></div></div>
    </div>
    <script>
        let selectedIndex = 0;
        let selectedChunkIdx = 0;
        let metatilesData = {json.dumps(STUDIO["metatiles"])};
        let metatileAttrs = {json.dumps(STUDIO["metatile_attrs"])};
        const attrBytes = {STUDIO["attr_bytes"]};
        const totalRawTiles = {total_raw_tiles};
        const layerTypesMap = {json.dumps(LAYER_TYPES)};
        const behaviorMap = {json.dumps(STUDIO["behavior_map"])};
        const terrainTypesMap = {json.dumps(TERRAIN_TYPES)};
        const encounterTypesMap = {json.dumps(ENCOUNTER_TYPES)};

        const chunkLabels = ["L1 Top-Left", "L1 Top-Right", "L1 Bot-Left", "L1 Bot-Right", "L2 Top-Left", "L2 Top-Right", "L2 Bot-Left", "L2 Bot-Right"];

        function populateDropdowns() {{
            const layerSel = document.getElementById('attr-layer');
            layerSel.innerHTML = '';
            for (let k in layerTypesMap) layerSel.innerHTML += `<option value="${{k}}">${{k}} - ${{layerTypesMap[k]}}</option>`;

            const behaviorSel = document.getElementById('attr-behavior');
            behaviorSel.innerHTML = '';
            let sortedBehaviors = [];
            for (let k in behaviorMap) sortedBehaviors.push({{ id: parseInt(k), name: behaviorMap[k] }});
            sortedBehaviors.sort((a, b) => a.id - b.id);

            if (sortedBehaviors.length === 0) {{
                for (let i = 0; i < 256; i++) sortedBehaviors.push({{ id: i, name: '0x' + i.toString(16).toUpperCase() }});
            }}

            sortedBehaviors.forEach(item => {{
                behaviorSel.innerHTML += `<option value="${{item.id}}">0x${{item.id.toString(16).toUpperCase().padStart(2, '0')}} (${{item.id}}) - ${{item.name}}</option>`;
            }});

            if (attrBytes === 4) {{
                document.getElementById('row-terrain').style.display = 'flex';
                document.getElementById('row-encounter').style.display = 'flex';
                const terrainSel = document.getElementById('attr-terrain');
                terrainSel.innerHTML = '';
                for (let k in terrainTypesMap) terrainSel.innerHTML += `<option value="${{k}}">${{k}} - ${{terrainTypesMap[k]}}</option>`;
                const encounterSel = document.getElementById('attr-encounter');
                encounterSel.innerHTML = '';
                for (let k in encounterTypesMap) encounterSel.innerHTML += `<option value="${{k}}">${{k}} - ${{encounterTypesMap[k]}}</option>`;
            }}
        }}

        function parseTile(val) {{
            return {{ id: val & 0x03FF, xFlip: Boolean(val & 0x0400), yFlip: Boolean(val & 0x0800), pal: (val >> 12) & 0x0F }};
        }}

        function packTile(id, xFlip, yFlip, pal) {{
            return ((id & 0x03FF) | (xFlip ? 0x0400 : 0) | (yFlip ? 0x0800 : 0) | ((pal & 0x0F) << 12)) & 0xFFFF;
        }}

        function initGrid() {{
            const grid = document.getElementById('grid');
            grid.innerHTML = '';
            for (let i = 0; i < metatilesData.length; i++) {{
                const card = document.createElement('div');
                card.className = 'metatile-card' + (i === selectedIndex ? ' selected' : '');
                card.id = 'meta-' + i;
                card.onclick = () => selectMetatile(i);
                
                const img = document.createElement('img');
                img.id = 'meta-img-' + i;
                img.src = '/metatile/' + i + '.png';
                
                const label = document.createElement('div');
                label.className = 'metatile-id';
                label.innerText = i;

                card.appendChild(img);
                card.appendChild(label);
                grid.appendChild(card);
            }}
            if (metatilesData.length > 0) selectMetatile(Math.min(Math.max(selectedIndex, 0), metatilesData.length - 1));
        }}

        function renderAtlas() {{
            const atlas = document.getElementById('raw-atlas');
            atlas.innerHTML = '';
            for (let i = 0; i < totalRawTiles; i++) {{
                const tileVal = packTile(i, false, false, 0);
                const cell = document.createElement('img');
                cell.className = 'raw-tile-cell';
                cell.src = '/tile/' + tileVal + '.png';
                cell.title = 'Tile ID: ' + i;
                cell.onclick = () => updateChunkProperty(selectedChunkIdx, 'id', i);
                atlas.appendChild(cell);
            }}
        }}

        function addMetatile() {{
            metatilesData.push([0, 0, 0, 0, 0, 0, 0, 0]);
            metatileAttrs.push({{ behavior: 0, layer: 0, terrain: 0, encounter: 0 }});
            selectedIndex = metatilesData.length - 1;
            selectedChunkIdx = 0;
            initGrid();
            document.getElementById('metatile-count').innerText = metatilesData.length;
            selectMetatile(selectedIndex);
        }}

        function deleteMetatile() {{
            if (metatilesData.length === 0) return;
            if (!confirm(`Delete metatile #${{selectedIndex}}?`)) return;
            metatilesData.splice(selectedIndex, 1);
            metatileAttrs.splice(selectedIndex, 1);
            selectedIndex = Math.max(0, Math.min(selectedIndex, metatilesData.length - 1));
            document.getElementById('metatile-count').innerText = metatilesData.length;
            initGrid();
        }}

        function selectMetatile(id) {{
            if (selectedIndex !== null) {{
                const prev = document.getElementById('meta-' + selectedIndex);
                if (prev) prev.classList.remove('selected');
            }}
            selectedIndex = id;
            const current = document.getElementById('meta-' + id);
            if (current) current.classList.add('selected');

            document.getElementById('selected-img').src = '/metatile/' + id + '.png?t=' + Date.now();
            document.getElementById('selected-info').innerText = 'Metatile #' + id + ' (0x' + id.toString(16).toUpperCase() + ')';

            const attr = metatileAttrs[id] || {{ behavior: 0, layer: 0, terrain: 0, encounter: 0 }};
            document.getElementById('attr-layer').value = attr.layer;
            document.getElementById('attr-behavior').value = attr.behavior;
            if (attrBytes === 4) {{
                document.getElementById('attr-terrain').value = attr.terrain;
                document.getElementById('attr-encounter').value = attr.encounter;
            }}

            renderChunkCards();
        }}

        function updateAttrProperty(prop, val) {{
            if (!metatileAttrs[selectedIndex]) metatileAttrs[selectedIndex] = {{ behavior: 0, layer: 0, terrain: 0, encounter: 0 }};
            metatileAttrs[selectedIndex][prop] = parseInt(val) || 0;
        }}

        function selectChunk(chunkIdx) {{
            selectedChunkIdx = chunkIdx;
            renderChunkCards();
        }}

        function renderChunkCards() {{
            const container = document.getElementById('chunks-container');
            container.innerHTML = '';
            const entry = metatilesData[selectedIndex] || [0,0,0,0,0,0,0,0];

            for (let i = 0; i < 8; i++) {{
                const tileVal = entry[i] || 0;
                const parsed = parseTile(tileVal);

                const card = document.createElement('div');
                card.className = 'chunk-card' + (i === selectedChunkIdx ? ' selected' : '');
                card.onclick = (e) => {{
                    if (e.target.tagName !== 'INPUT' && e.target.tagName !== 'SELECT' && e.target.tagName !== 'BUTTON') selectChunk(i);
                }};

                const header = document.createElement('div');
                header.className = 'chunk-header';
                header.innerHTML = `<span>#${{i}} ${{chunkLabels[i]}}</span>`;

                const body = document.createElement('div');
                body.className = 'chunk-body';

                const img = document.createElement('img');
                img.className = 'chunk-preview';
                img.id = 'chunk-img-' + i;
                img.src = '/tile/' + tileVal + '.png';

                const controls = document.createElement('div');
                controls.className = 'chunk-controls';

                const idRow = document.createElement('div');
                idRow.style.display = 'flex'; idRow.style.gap = '4px'; idRow.style.alignItems = 'center';
                idRow.innerHTML = `<label>ID:</label><input type="number" min="0" max="1023" value="${{parsed.id}}" onchange="updateChunkProperty(${{i}}, 'id', this.value)">`;

                const palRow = document.createElement('div');
                palRow.style.display = 'flex'; palRow.style.gap = '4px'; palRow.style.alignItems = 'center';
                let palOptions = '';
                for (let p = 0; p < 16; p++) palOptions += `<option value="${{p}}" ${{parsed.pal === p ? 'selected' : ''}}>Pal ${{p}}</option>`;
                palRow.innerHTML = `<label>Pal:</label><select onchange="updateChunkProperty(${{i}}, 'pal', this.value)">${{palOptions}}</select>`;

                const flipRow = document.createElement('div');
                flipRow.style.display = 'flex'; flipRow.style.gap = '4px'; flipRow.style.marginTop = '2px';
                flipRow.innerHTML = `
                    <button class="btn-toggle ${{parsed.xFlip ? 'on' : ''}}" onclick="updateChunkProperty(${{i}}, 'xFlip', ${{!parsed.xFlip}})">H-Flip</button>
                    <button class="btn-toggle ${{parsed.yFlip ? 'on' : ''}}" onclick="updateChunkProperty(${{i}}, 'yFlip', ${{!parsed.yFlip}})">V-Flip</button>
                `;

                controls.appendChild(idRow);
                controls.appendChild(palRow);
                controls.appendChild(flipRow);

                body.appendChild(img);
                body.appendChild(controls);
                card.appendChild(header);
                card.appendChild(body);
                container.appendChild(card);
            }}
        }}

        function updateChunkProperty(chunkIdx, prop, val) {{
            const entry = metatilesData[selectedIndex];
            const parsed = parseTile(entry[chunkIdx] || 0);

            if (prop === 'id') parsed.id = parseInt(val) || 0;
            if (prop === 'pal') parsed.pal = parseInt(val) || 0;
            if (prop === 'xFlip') parsed.xFlip = Boolean(val);
            if (prop === 'yFlip') parsed.yFlip = Boolean(val);

            entry[chunkIdx] = packTile(parsed.id, parsed.xFlip, parsed.yFlip, parsed.pal);

            const timestamp = Date.now();
            const chunkImg = document.getElementById('chunk-img-' + chunkIdx);
            if (chunkImg) chunkImg.src = '/tile/' + entry[chunkIdx] + '.png?t=' + timestamp;

            document.getElementById('selected-img').src = '/metatile/' + selectedIndex + '.png?t=' + timestamp;
            document.getElementById('meta-img-' + selectedIndex).src = '/metatile/' + selectedIndex + '.png?t=' + timestamp;

            selectChunk(chunkIdx);
        }}

        async function saveMetatiles() {{
            const res = await fetch('/save', {{
                method: 'POST',
                headers: {{ 'Content-Type': 'application/json' }},
                body: JSON.stringify({{ metatiles: metatilesData, attributes: metatileAttrs }})
            }});
            if (res.ok) alert('Metatiles and Attributes saved successfully!');
            else alert('Failed to save metatiles.');
        }}

        populateDropdowns();
        initGrid();
        renderAtlas();
    </script>
</body>
</html>"""
            self.wfile.write(html.encode("utf-8"))
            return

        elif self.path.startswith("/metatile/"):
            match = re.match(r"^/metatile/(\d+)\.png", self.path)
            if match:
                idx = int(match.group(1))
                if 0 <= idx < len(STUDIO["metatiles"]):
                    img = render_metatile(STUDIO["metatiles"][idx])
                    buf = io.BytesIO()
                    img.save(buf, format="PNG")
                    img_data = buf.getvalue()

                    self.send_response(200)
                    self.send_header("Content-Type", "image/png")
                    self.send_header("Content-Length", str(len(img_data)))
                    self.end_headers()
                    self.wfile.write(img_data)
                    return

        elif self.path.startswith("/tile/"):
            match = re.match(r"^/tile/(\d+)\.png", self.path)
            if match:
                tile_u16 = int(match.group(1))
                img = render_8x8_tile(tile_u16)
                buf = io.BytesIO()
                img.save(buf, format="PNG")
                img_data = buf.getvalue()

                self.send_response(200)
                self.send_header("Content-Type", "image/png")
                self.send_header("Content-Length", str(len(img_data)))
                self.end_headers()
                self.wfile.write(img_data)
                return

        self.send_response(404)
        self.end_headers()

    def do_POST(self):
        if self.path == "/save":
            content_length = int(self.headers.get("Content-Length", 0))
            if content_length > 0:
                body = self.rfile.read(content_length).decode("utf-8")
                data = json.loads(body)
                if "metatiles" in data:
                    STUDIO["metatiles"] = data["metatiles"]
                if "attributes" in data:
                    STUDIO["metatile_attrs"] = data["attributes"]

            if save_metatiles_to_disk():
                res_msg = json.dumps(
                    {"message": "Successfully updated metatiles and attributes."}
                ).encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(res_msg)))
                self.end_headers()
                self.wfile.write(res_msg)
                return

            self.send_response(500)
            self.end_headers()


def main():
    parser = argparse.ArgumentParser(
        description="Pokeemerald Metatile Editor Studio"
    )
    parser.add_argument("dir_path", help="Path to target tileset folder")
    parser.add_argument(
        "--header",
        default="../include/constants/metatile_behaviors.h",
        help="Path to metatile_behaviors.h",
    )
    parser.add_argument(
        "-b",
        "--bytes",
        type=int,
        choices=[2, 4],
        default=2,
        help="Attribute bytes per entry",
    )
    parser.add_argument(
        "--primary-offset",
        type=int,
        default=512,
        help="Tile ID threshold for secondary tileset subtraction",
    )
    args = parser.parse_args()

    stage_tileset(
        args.dir_path, args.header, args.bytes, primary_tile_count=args.primary_offset
    )

    server = ThreadingHTTPServer(("0.0.0.0", 8080), TilesetWebBackend)
    print("GBA Metatile Editor active at http://localhost:8080")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")


if __name__ == "__main__":
    main()
