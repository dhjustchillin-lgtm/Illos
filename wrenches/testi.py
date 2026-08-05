import sys
import os
import re
import argparse
import io
import json
import glob
import struct
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    from PIL import Image, ImageOps
except ImportError:
    print("[CRITICAL] 'Pillow' library is required. Run: pip install Pillow")
    sys.exit(1)

STUDIO = {
    "dir_path": "",
    "tiles_png_path": "",
    "metatiles_bin_path": "",
    "attr_bin_path": "",
    "palettes": {},           # Map of pal_idx (0-15) -> list of 16 RGB tuples
    "tiles_img": None,        # Raw 8x8 indexed tiles image
    "metatiles": [],          # List of 8-u16 entries per metatile [L1_TL, L1_TR, L1_BL, L1_BR, L2_TL, L2_TR, L2_BL, L2_BR]
    "metatile_attrs": []      # List of u16 attribute values per metatile
}

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
                        colors.append((int(parts[0]), int(parts[1]), int(parts[2])))
            else:
                for line in lines:
                    match = re.match(r'^(\d+)\s+(\d+)\s+(\d+)$', line)
                    if match:
                        colors.append((int(match.group(1)), int(match.group(2)), int(match.group(3))))
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
        default_pal = [(0, 0, 0)] + [(i * 16, i * 16, i * 16) for i in range(1, 16)]
        for i in range(16):
            palettes[i] = list(default_pal)

    return palettes

def stage_tileset(dir_path):
    STUDIO["dir_path"] = dir_path
    
    png_files = glob.glob(os.path.join(dir_path, "*.png"))
    STUDIO["tiles_png_path"] = png_files[0] if png_files else os.path.join(dir_path, "tiles.png")
    STUDIO["metatiles_bin_path"] = os.path.join(dir_path, "metatiles.bin")
    STUDIO["attr_bin_path"] = os.path.join(dir_path, "metatile_attributes.bin")

    STUDIO["palettes"] = load_all_palettes(dir_path)

    if os.path.exists(STUDIO["tiles_png_path"]):
        STUDIO["tiles_img"] = Image.open(STUDIO["tiles_png_path"]).convert("P")
    else:
        STUDIO["tiles_img"] = Image.new("P", (128, 128), 0)

    STUDIO["metatiles"] = []
    if os.path.exists(STUDIO["metatiles_bin_path"]):
        with open(STUDIO["metatiles_bin_path"], "rb") as f:
            data = f.read()
            num_entries = len(data) // 2
            u16_data = struct.unpack(f'<{num_entries}H', data)
            
            stride = 8 if num_entries % 8 == 0 else 4
            for i in range(0, len(u16_data), stride):
                entry = list(u16_data[i:i+stride])
                if len(entry) < 8:
                    entry = entry + [0] * (8 - len(entry))
                STUDIO["metatiles"].append(entry)

    STUDIO["metatile_attrs"] = []
    if os.path.exists(STUDIO["attr_bin_path"]):
        with open(STUDIO["attr_bin_path"], "rb") as f:
            data = f.read()
            if len(data) % 2 == 0:
                num_attrs = len(data) // 2
                STUDIO["metatile_attrs"] = list(struct.unpack(f'<{num_attrs}H', data))

    while len(STUDIO["metatile_attrs"]) < len(STUDIO["metatiles"]):
        STUDIO["metatile_attrs"].append(0)

def render_8x8_tile(tile_u16):
    tile_id = tile_u16 & 0x03FF
    x_flip = bool(tile_u16 & 0x0400)
    y_flip = bool(tile_u16 & 0x0800)
    pal_idx = (tile_u16 >> 12) & 0x0F

    tiles_img = STUDIO["tiles_img"]
    tiles_per_row = max(1, tiles_img.width // 8)
    
    col = tile_id % tiles_per_row
    row = tile_id // tiles_per_row

    crop_box = (col * 8, row * 8, (col + 1) * 8, (row + 1) * 8)
    
    if crop_box[2] <= tiles_img.width and crop_box[3] <= tiles_img.height:
        tile_crop = tiles_img.crop(crop_box)
    else:
        tile_crop = Image.new("P", (8, 8), 0)

    pal_colors = STUDIO["palettes"].get(pal_idx, STUDIO["palettes"].get(0, [(0,0,0)]*16))
    flattened_pal = []
    for rgb in pal_colors:
        flattened_pal.extend(rgb)
    while len(flattened_pal) < 768:
        flattened_pal.extend([0, 0, 0])

    tile_rgb = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
    tile_crop_p = tile_crop.copy()
    tile_crop_p.putpalette(flattened_pal)
    tile_rgba = tile_crop_p.convert("RGBA")

    pixels = tile_crop.getdata()
    rgba_pixels = list(tile_rgba.getdata())
    final_pixels = []
    for idx, pixel_val in enumerate(pixels):
        if pixel_val % 16 == 0:
            final_pixels.append((0, 0, 0, 0))
        else:
            final_pixels.append(rgba_pixels[idx])
            
    tile_rgb.putdata(final_pixels)

    if x_flip:
        tile_rgb = ImageOps.mirror(tile_rgb)
    if y_flip:
        tile_rgb = ImageOps.flip(tile_rgb)

    return tile_rgb

def render_metatile(metatile_entry):
    img = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    positions = [(0, 0), (8, 0), (0, 8), (8, 8)]

    # Layer 1 (Bottom)
    for idx in range(4):
        tile_u16 = metatile_entry[idx]
        sub_tile = render_8x8_tile(tile_u16)
        img.paste(sub_tile, positions[idx], sub_tile)

    # Layer 2 (Top)
    for idx in range(4):
        tile_u16 = metatile_entry[4 + idx]
        if tile_u16 & 0x03FF != 0:
            sub_tile = render_8x8_tile(tile_u16)
            img.paste(sub_tile, positions[idx], sub_tile)

    return img

def save_metatiles_to_disk():
    try:
        u16_list = []
        for meta in STUDIO["metatiles"]:
            u16_list.extend(meta[:8])
        
        # Save metatiles binary in binary write mode ("wb")
        with open(STUDIO["metatiles_bin_path"], "wb") as f:
            f.write(struct.pack(f'<{len(u16_list)}H', *u16_list))

        # FIX: Open in binary mode ("wb") for attributes as well
        if STUDIO["metatile_attrs"]:
            with open(STUDIO["attr_bin_path"], "wb") as f:
                f.write(struct.pack(f'<{len(STUDIO["metatile_attrs"])}H', *STUDIO["metatile_attrs"]))

        print(f"[SUCCESS] Saved {len(STUDIO['metatiles'])} metatiles to {STUDIO['metatiles_bin_path']}")
        return True
    except Exception as e:
        print(f"[ERROR] Failed to save metatiles: {e}")
        return False


class TilesetWebBackend(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()

            total_raw_tiles = 0
            if STUDIO["tiles_img"]:
                total_raw_tiles = (STUDIO["tiles_img"].width // 8) * (STUDIO["tiles_img"].height // 8)

            html = f"""<!DOCTYPE html>
<html>
<head>
    <title>Pokeemerald Metatile Studio</title>
    <style>
        body {{
            margin: 0;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #1e1e1e;
            color: #d4d4d4;
            display: flex;
            height: 100vh;
            overflow: hidden;
        }}
        #sidebar {{
            width: 420px;
            background: #252526;
            border-right: 1px solid #3c3c3c;
            display: flex;
            flex-direction: column;
            padding: 15px;
            box-sizing: border-box;
            overflow-y: auto;
        }}
        #main {{
            flex: 1;
            display: flex;
            flex-direction: column;
            background: #1e1e1e;
        }}
        #toolbar {{
            height: 50px;
            background: #2d2d2d;
            border-bottom: 1px solid #3c3c3c;
            display: flex;
            align-items: center;
            padding: 0 20px;
            gap: 10px;
        }}
        #content {{
            flex: 1;
            overflow: auto;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: flex-start;
        }}
        .grid-container {{
            display: grid;
            grid-template-columns: repeat(8, 64px);
            gap: 4px;
            background: #111;
            padding: 10px;
            border-radius: 6px;
            box-shadow: 0 4px 10px rgba(0,0,0,0.5);
        }}
        .metatile-card {{
            width: 64px;
            height: 64px;
            background: #2a2a2a;
            border: 2px solid #3c3c3c;
            border-radius: 4px;
            cursor: pointer;
            position: relative;
            image-rendering: pixelated;
        }}
        .metatile-card:hover {{
            border-color: #0e639c;
        }}
        .metatile-card.selected {{
            border-color: #007acc;
            box-shadow: 0 0 8px #007acc;
        }}
        .metatile-card img {{
            width: 100%;
            height: 100%;
            image-rendering: pixelated;
        }}
        .metatile-id {{
            position: absolute;
            bottom: 2px;
            right: 2px;
            background: rgba(0,0,0,0.7);
            font-size: 10px;
            padding: 1px 3px;
            border-radius: 2px;
        }}
        button {{
            background: #0e639c;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            font-size: 11px;
        }}
        button:hover {{
            background: #1177bb;
        }}
        button.active {{
            background: #007acc;
            border: 1px solid #fff;
        }}
        .sidebar-section {{
            margin-bottom: 15px;
        }}
        .sidebar-title {{
            font-size: 12px;
            font-weight: bold;
            color: #888;
            text-transform: uppercase;
            margin-bottom: 8px;
            border-bottom: 1px solid #333;
            padding-bottom: 4px;
        }}
        .preview-box {{
            width: 128px;
            height: 128px;
            border: 2px solid #3c3c3c;
            background: #000;
            margin: 0 auto 10px auto;
            image-rendering: pixelated;
        }}
        .preview-box img {{
            width: 100%;
            height: 100%;
            image-rendering: pixelated;
        }}
        .chunks-grid {{
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 6px;
        }}
        .chunk-card {{
            background: #1e1e1e;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            padding: 6px;
            display: flex;
            flex-direction: column;
            gap: 4px;
            cursor: pointer;
        }}
        .chunk-card.selected {{
            border-color: #007acc;
            background: #2d2d30;
        }}
        .chunk-header {{
            display: flex;
            align-items: center;
            justify-content: space-between;
            font-size: 11px;
            font-weight: bold;
            color: #ccc;
        }}
        .chunk-body {{
            display: flex;
            gap: 8px;
            align-items: center;
        }}
        .chunk-preview {{
            width: 32px;
            height: 32px;
            border: 1px solid #444;
            background: #000;
            image-rendering: pixelated;
            flex-shrink: 0;
        }}
        .chunk-controls {{
            display: flex;
            flex-direction: column;
            gap: 3px;
            flex: 1;
        }}
        .chunk-controls label {{
            font-size: 10px;
            color: #aaa;
            width: 35px;
        }}
        .chunk-controls input[type="number"], .chunk-controls select {{
            background: #111;
            border: 1px solid #444;
            color: #fff;
            font-size: 11px;
            padding: 2px 4px;
            border-radius: 2px;
            width: 100%;
            box-sizing: border-box;
        }}
        .btn-toggle {{
            padding: 2px 5px;
            font-size: 10px;
            background: #333;
            border: 1px solid #555;
            color: #ccc;
        }}
        .btn-toggle.on {{
            background: #0e639c;
            color: #fff;
            border-color: #007acc;
        }}
        .raw-atlas-wrapper {{
            overflow-x: auto;
            max-height: 200px;
            overflow-y: auto;
            background: #111;
            padding: 4px;
            border: 1px solid #333;
            border-radius: 4px;
        }}
        .raw-atlas-grid {{
            display: grid;
            grid-template-columns: repeat(16, 22px);
            gap: 2px;
            width: max-content;
        }}
        .raw-tile-cell {{
            width: 22px;
            height: 22px;
            background: #222;
            border: 1px solid #333;
            cursor: pointer;
            box-sizing: border-box;
            image-rendering: pixelated;
        }}
        .raw-tile-cell:hover {{
            border-color: #007acc;
        }}
        .raw-tile-cell.active {{
            border-color: #fff;
            box-shadow: 0 0 4px #fff;
        }}
        .save-btn {{
            width: 100%;
            padding: 10px;
            margin-top: 10px;
            background: #28a745;
            color: #fff;
            font-size: 12px;
            font-weight: bold;
            border-radius: 4px;
            border: none;
            cursor: pointer;
        }}
        .save-btn:hover {{
            background: #218838;
        }}
    </style>
</head>
<body>
    <div id="sidebar">
        <div class="sidebar-section">
            <div class="sidebar-title">Selected Metatile</div>
            <div class="preview-box">
                <img id="selected-img" src="" alt="Select Metatile">
            </div>
            <div id="selected-info" style="text-align: center; font-weight: bold; margin-bottom: 10px;">Select a Metatile</div>
        </div>

        <div class="sidebar-section">
            <div class="sidebar-title">16 Per Row Raw Tile Atlas</div>
            <div class="raw-atlas-wrapper">
                <div class="raw-atlas-grid" id="raw-atlas"></div>
            </div>
        </div>

        <div class="sidebar-section">
            <div class="sidebar-title">8x8 Sub-Tile Chunks Breakdown</div>
            <div style="font-size: 11px; color: #888; margin-bottom: 6px;">Layer 1 (Bottom) & Layer 2 (Top)</div>
            <div class="chunks-grid" id="chunks-container"></div>
            <button class="save-btn" onclick="saveMetatiles()">💾 SAVE CHANGES TO DISK</button>
        </div>
    </div>

    <div id="main">
        <div id="toolbar">
            <span style="font-weight: bold;">Tileset Target:</span>
            <span>{os.path.basename(STUDIO["dir_path"])}</span>
            <span style="margin-left: auto; color: #aaa;">Total Metatiles: {len(STUDIO["metatiles"])}</span>
        </div>
        <div id="content">
            <div class="grid-container" id="grid"></div>
        </div>
    </div>

    <script>
        let selectedIndex = 0;
        let selectedChunkIdx = 0;
        let metatilesData = {json.dumps(STUDIO["metatiles"])};
        const totalRawTiles = {total_raw_tiles};

        const chunkLabels = [
            "L1 Top-Left", "L1 Top-Right", "L1 Bot-Left", "L1 Bot-Right",
            "L2 Top-Left", "L2 Top-Right", "L2 Bot-Left", "L2 Bot-Right"
        ];

        function parseTile(val) {{
            return {{
                id: val & 0x03FF,
                xFlip: Boolean(val & 0x0400),
                yFlip: Boolean(val & 0x0800),
                pal: (val >> 12) & 0x0F
            }};
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
            selectMetatile(0);
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
                cell.onclick = () => {{
                    updateChunkProperty(selectedChunkIdx, 'id', i);
                }};
                atlas.appendChild(cell);
            }}
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
            document.getElementById('selected-info').innerText = 'Metatile #' + id;

            renderChunkCards();
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
                    if (e.target.tagName !== 'INPUT' && e.target.tagName !== 'SELECT' && e.target.tagName !== 'BUTTON') {{
                        selectChunk(i);
                    }}
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

                // Tile ID
                const idRow = document.createElement('div');
                idRow.style.display = 'flex';
                idRow.style.gap = '4px';
                idRow.style.alignItems = 'center';
                idRow.innerHTML = `<label>ID:</label><input type="number" min="0" max="1023" value="${{parsed.id}}" onchange="updateChunkProperty(${{i}}, 'id', this.value)">`;

                // Palette dropdown
                const palRow = document.createElement('div');
                palRow.style.display = 'flex';
                palRow.style.gap = '4px';
                palRow.style.alignItems = 'center';
                let palOptions = '';
                for (let p = 0; p < 16; p++) {{
                    palOptions += `<option value="${{p}}" ${{parsed.pal === p ? 'selected' : ''}}>Pal ${{p}}</option>`;
                }}
                palRow.innerHTML = `<label>Pal:</label><select onchange="updateChunkProperty(${{i}}, 'pal', this.value)">${{palOptions}}</select>`;

                // Flips
                const flipRow = document.createElement('div');
                flipRow.style.display = 'flex';
                flipRow.style.gap = '4px';
                flipRow.style.marginTop = '2px';
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

            // Live refresh chunk preview image
            const chunkImg = document.getElementById('chunk-img-' + chunkIdx);
            if (chunkImg) chunkImg.src = '/tile/' + entry[chunkIdx] + '.png?t=' + Date.now();

            // Live refresh metatile images
            const timestamp = Date.now();
            document.getElementById('selected-img').src = '/metatile/' + selectedIndex + '.png?t=' + timestamp;
            document.getElementById('meta-img-' + selectedIndex).src = '/metatile/' + selectedIndex + '.png?t=' + timestamp;

            selectChunk(chunkIdx);
        }}

        async function saveMetatiles() {{
            const res = await fetch('/save', {{
                method: 'POST',
                headers: {{ 'Content-Type': 'application/json' }},
                body: JSON.stringify({{ metatiles: metatilesData }})
            }});
            if (res.ok) {{
                alert('Metatiles saved successfully!');
            }} else {{
                alert('Failed to save metatiles.');
            }}
        }}

        initGrid();
        renderAtlas();
    </script>
</body>
</html>"""
            self.wfile.write(html.encode("utf-8"))
            return

        elif self.path.startswith("/metatile/"):
            match = re.match(r'^/metatile/(\d+)\.png', self.path)
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
            match = re.match(r'^/tile/(\d+)\.png', self.path)
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
            content_length = int(self.headers.get('Content-Length', 0))
            if content_length > 0:
                body = self.rfile.read(content_length).decode('utf-8')
                data = json.loads(body)
                if "metatiles" in data:
                    STUDIO["metatiles"] = data["metatiles"]

            if save_metatiles_to_disk():
                res_msg = json.dumps({"message": f"Successfully updated metatiles binary at '{STUDIO['metatiles_bin_path']}'"}).encode('utf-8')
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(res_msg)))
                self.end_headers()
                self.wfile.write(res_msg)
                return

            self.send_response(500)
            self.end_headers()

def main():
    parser = argparse.ArgumentParser(description="Pokeemerald Metatile Editor Studio")
    parser.add_argument("dir_path", help="Path to target tileset folder")
    args = parser.parse_args()

    stage_tileset(args.dir_path)

    server = ThreadingHTTPServer(('0.0.0.0', 8080), TilesetWebBackend)
    print("GBA Metatile Editor active at http://localhost:8080")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")

if __name__ == "__main__":
    main()
