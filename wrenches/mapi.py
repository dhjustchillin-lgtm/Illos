import sys
import os
import csv
import json
import re
import argparse
import io
import shutil
# Changed HTTPServer to ThreadingHTTPServer to handle concurrent requests
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    from PIL import Image, ImageOps
except ImportError:
    print("[CRITICAL] 'Pillow' library is required. Run: pip install Pillow")
    sys.exit(1)

STUDIO = {
    "root_dir": "",
    "allowed_tilesets": [],
    "secondary_offset": 512,
    "palettes": {},
    "maps": {}
}

def normalize_tileset_name(ts_name):
    if not ts_name: 
        return ""
    if ts_name.startswith("gTileset_"):
        ts_name = ts_name[len("gTileset_"):]
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', re.sub('(.)([A-Z][a-z]+)', r'\1_\2', ts_name)).lower()

def load_gba_pal_file(filepath):
    colors = []
    if not os.path.exists(filepath): 
        return None
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            for line in f:
                match = re.match(r'^(\d+)\s+(\d+)\s+(\d+)$', line.strip())
                if match:
                    colors.append((int(match.group(1)), int(match.group(2)), int(match.group(3))))
        return colors if len(colors) >= 16 else None
    except Exception: 
        return None

def load_tileset_palettes(root_dir, domain, ts_folder):
    domain_pals = {}
    pal_dir = os.path.join(root_dir, "data", "tilesets", domain, ts_folder, "palettes")
    if os.path.exists(pal_dir):
        for file in os.listdir(pal_dir):
            if file.endswith(".pal"):
                num_match = re.search(r'(\d+)', file)
                if num_match:
                    pal_idx = int(num_match.group(1))
                    colors = load_gba_pal_file(os.path.join(pal_dir, file))
                    if colors: 
                        domain_pals[pal_idx] = colors
    return domain_pals

def inject_script_into_event_scripts(root_dir, map_name):
    event_scripts_path = os.path.join(root_dir, "data", "event_scripts.s")
    if not os.path.exists(event_scripts_path):
        return

    include_line = f'\t.include "data/maps/{map_name}/scripts.inc"\n'
    try:
        with open(event_scripts_path, "r", encoding="utf-8") as f:
            content = f.read()
        if include_line not in content:
            with open(event_scripts_path, "a", encoding="utf-8") as f:
                f.write("\n" + include_line)
    except Exception as e:
        print(f"[ERROR] Failed to append script include to event_scripts.s: {e}")

def create_blank_map_structure(root_dir, map_name):
    print(f"[INFO] Initializing new map files from template for: {map_name}")

    # Kept underscores out of the clean_name logic to avoid casing collisions
    clean_name = map_name.replace(" ", "")
    layout_id = f"LAYOUT_{map_name.upper()}"
    map_id = f"MAP_{map_name.upper()}"

    layout_dir = os.path.join(root_dir, "data", "layouts", clean_name)
    map_dir = os.path.join(root_dir, "data", "maps", map_name)
    layouts_json_path = os.path.join(root_dir, "data", "layouts", "layouts.json")
    groups_json_path = os.path.join(root_dir, "data", "maps", "groups.json")

    os.makedirs(layout_dir, exist_ok=True)
    os.makedirs(map_dir, exist_ok=True)

    map_bin = os.path.join(layout_dir, "map.bin")
    border_bin = os.path.join(layout_dir, "border.bin")

    if not os.path.exists(map_bin):
        with open(map_bin, "wb") as f:
            f.write(bytearray([0] * (20 * 20 * 2)))

    if not os.path.exists(border_bin):
        with open(border_bin, "wb") as f:
            f.write(bytearray([0] * (2 * 2 * 2)))

    scripts_inc = os.path.join(map_dir, "scripts.inc")
    if not os.path.exists(scripts_inc):
        with open(scripts_inc, "w", encoding="utf-8") as f:
            f.write(f"{clean_name}_MapScripts::\n\t.byte 0\n")

    inject_script_into_event_scripts(root_dir, map_name)

    map_json_data = {
        "id": map_id,
        "name": map_name,
        "layout": layout_id,
        "music": "MUS_LITTLEROOT",
        "region_map_section": "MAPSEC_LITTLEROOT_TOWN",
        "requires_flash": False,
        "weather": "WEATHER_SUNNY",
        "map_type": "MAP_TYPE_TOWN",
        "allow_cycling": True,
        "allow_escaping": False,
        "allow_running": True,
        "show_map_name": True,
        "battle_scene": "MAP_BATTLE_SCENE_NORMAL",
        "connections": None,
        "object_events": [],
        "warp_events": [],
        "coord_events": [],
        "bg_events": []
    }

    with open(os.path.join(map_dir, "map.json"), "w", encoding="utf-8") as f:
        json.dump(map_json_data, f, indent=4)

    if os.path.exists(layouts_json_path):
        try:
            with open(layouts_json_path, "r", encoding="utf-8") as f:
                layouts_data = json.load(f)

            exists = any(item.get("id") == layout_id for item in layouts_data.get("layouts", []))
            if not exists:
                layouts_data.setdefault("layouts", []).append({
                    "id": layout_id,
                    "name": f"{clean_name}_Layout",
                    "width": 20,
                    "height": 20,
                    "primary_tileset": "gTileset_General",
                    "secondary_tileset": "gTileset_Petalburg",
                    "border_filepath": f"data/layouts/{clean_name}/border.bin",
                    "blockdata_filepath": f"data/layouts/{clean_name}/map.bin"
                })
                with open(layouts_json_path, "w", encoding="utf-8") as f:
                    json.dump(layouts_data, f, indent=4)
        except Exception as e:
            print(f"[ERROR] Failed updating layouts.json: {e}")

    if os.path.exists(groups_json_path):
        try:
            with open(groups_json_path, "r", encoding="utf-8") as f:
                groups_data = json.load(f)

            map_placed = False
            for group in groups_data.get("groups", []):
                for m in group.get("maps", []):
                    if m == map_id:
                        map_placed = True
                        break

            if not map_placed and len(groups_data.get("groups", [])) > 0:
                groups_data["groups"][0].setdefault("maps", []).append(map_id)
                with open(groups_json_path, "w", encoding="utf-8") as f:
                    json.dump(groups_data, f, indent=4)
        except Exception as e:
            print(f"[ERROR] Failed updating groups.json: {e}")

def delete_map_structure(root_dir, map_name):
    print(f"[INFO] Tearing down map structure for: {map_name}")

    clean_name = map_name.replace(" ", "")
    layout_id = f"LAYOUT_{map_name.upper()}"
    map_id = f"MAP_{map_name.upper()}"

    layout_dir = os.path.join(root_dir, "data", "layouts", clean_name)
    map_dir = os.path.join(root_dir, "data", "maps", map_name)
    layouts_json_path = os.path.join(root_dir, "data", "layouts", "layouts.json")
    groups_json_path = os.path.join(root_dir, "data", "maps", "groups.json")
    event_scripts_path = os.path.join(root_dir, "data", "event_scripts.s")

    if os.path.exists(map_dir):
        shutil.rmtree(map_dir)
        print(f"[INFO] Deleted map directory: {map_dir}")
    if os.path.exists(layout_dir):
        shutil.rmtree(layout_dir)
        print(f"[INFO] Deleted layout directory: {layout_dir}")

    if os.path.exists(event_scripts_path):
        include_line = f'\t.include "data/maps/{map_name}/scripts.inc"\n'
        try:
            with open(event_scripts_path, "r", encoding="utf-8") as f:
                content = f.read()
            if include_line in content:
                content = content.replace(f"\n{include_line}", "").replace(include_line, "")
                with open(event_scripts_path, "w", encoding="utf-8") as f:
                    f.write(content)
                print(f"[INFO] Removed include line from event_scripts.s")
        except Exception as e:
            print(f"[ERROR] Failed to clean event_scripts.s: {e}")

    if os.path.exists(layouts_json_path):
        try:
            with open(layouts_json_path, "r", encoding="utf-8") as f:
                layouts_data = json.load(f)
            if "layouts" in layouts_data:
                original_count = len(layouts_data["layouts"])
                layouts_data["layouts"] = [item for item in layouts_data["layouts"] if item.get("id") != layout_id]
                if len(layouts_data["layouts"]) < original_count:
                    with open(layouts_json_path, "w", encoding="utf-8") as f:
                        json.dump(layouts_data, f, indent=4)
                    print(f"[INFO] Removed layout entry from layouts.json")
        except Exception as e:
            print(f"[ERROR] Failed updating layouts.json during deletion: {e}")

    if os.path.exists(groups_json_path):
        try:
            with open(groups_json_path, "r", encoding="utf-8") as f:
                groups_data = json.load(f)
            if "groups" in groups_data:
                updated = False
                for group in groups_data["groups"]:
                    if "maps" in group and map_id in group["maps"]:
                        group["maps"].remove(map_id)
                        updated = True
                if updated:
                    with open(groups_json_path, "w", encoding="utf-8") as f:
                        json.dump(groups_data, f, indent=4)
                    print(f"[INFO] Removed map entry pointer from groups.json")
        except Exception as e:
            print(f"[ERROR] Failed updating groups.json during deletion: {e}")

def stage_map_into_studio(root_dir, map_name):
    if map_name in STUDIO["maps"]: 
        return True

    map_json_path = os.path.join(root_dir, "data", "maps", map_name, "map.json")
    layouts_json_path = os.path.join(root_dir, "data", "layouts", "layouts.json")

    if not os.path.exists(map_json_path) or not os.path.exists(layouts_json_path):
        create_blank_map_structure(root_dir, map_name)

    try:
        # Load object structures from map data to pipe directly into UI client
        with open(map_json_path, "r", encoding="utf-8") as f:
            map_config = json.load(f)
            layout_id = map_config.get("layout", "")
            object_events = map_config.get("object_events", [])
            warp_events = map_config.get("warp_events", [])
            coord_events = map_config.get("coord_events", [])
            bg_events = map_config.get("bg_events", [])

        width, height, p_ts, s_ts = 20, 20, "gTileset_General", "gTileset_Petalburg"
        blockdata_filepath, border_filepath = "", ""

        with open(layouts_json_path, "r", encoding="utf-8") as f:
            for item in json.load(f).get("layouts", []):
                if item.get("id") == layout_id:
                    width = int(item.get("width", 20))
                    height = int(item.get("height", 20))
                    p_ts = item.get("primary_tileset", "gTileset_General")
                    s_ts = item.get("secondary_tileset", "gTileset_Petalburg")
                    blockdata_filepath = item.get("blockdata_filepath", "")
                    border_filepath = item.get("border_filepath", "")
                    break

        p_folder = normalize_tileset_name(p_ts)
        s_folder = normalize_tileset_name(s_ts)

        if "primary" not in STUDIO["palettes"]: STUDIO["palettes"]["primary"] = {}
        if "secondary" not in STUDIO["palettes"]: STUDIO["palettes"]["secondary"] = {}

        if p_folder and p_folder not in STUDIO["palettes"]["primary"]:
            STUDIO["palettes"]["primary"][p_folder] = load_tileset_palettes(root_dir, "primary", p_folder)
        if s_folder and s_folder not in STUDIO["palettes"]["secondary"]:
            STUDIO["palettes"]["secondary"][s_folder] = load_tileset_palettes(root_dir, "secondary", s_folder)

        if blockdata_filepath:
            map_bin = os.path.join(root_dir, blockdata_filepath)
        else:
            layout_clean = layout_id.replace("LAYOUT_", "").replace(" ", "")
            map_bin = os.path.join(root_dir, "data", "layouts", layout_clean, "map.bin")

        if border_filepath:
            border_bin = os.path.join(root_dir, border_filepath)
        else:
            layout_clean = layout_id.replace("LAYOUT_", "").replace(" ", "")
            border_bin = os.path.join(root_dir, "data", "layouts", layout_clean, "border.bin")

        metatiles, border_blocks = [], []
        if os.path.exists(map_bin):
            with open(map_bin, "rb") as f:
                while (b := f.read(2)): 
                    metatiles.append(int.from_bytes(b, byteorder='little'))
        if os.path.exists(border_bin):
            with open(border_bin, "rb") as f:
                while (b := f.read(2)): 
                    border_blocks.append(int.from_bytes(b, byteorder='little'))

        STUDIO["maps"][map_name] = {
            "map_name": map_name, "layout_id": layout_id, "width": width, "height": height,
            "p_folder": p_folder, "s_folder": s_folder, "map_bin_path": map_bin, "border_bin_path": border_bin,
            "metatiles": metatiles, "border_blocks": border_blocks,
            "primary_tiles_png": os.path.join(root_dir, "data", "tilesets", "primary", p_folder, "tiles.png"),
            "primary_metatiles_bin": os.path.join(root_dir, "data", "tilesets", "primary", p_folder, "metatiles.bin"),
            "secondary_tiles_png": os.path.join(root_dir, "data", "tilesets", "secondary", s_folder, "tiles.png"),
            "secondary_metatiles_bin": os.path.join(root_dir, "data", "tilesets", "secondary", s_folder, "metatiles.bin"),
            "object_events": object_events, "warp_events": warp_events, "coord_events": coord_events, "bg_events": bg_events
        }
        return True
    except Exception as e:
        print(f"[ERROR] Failed staging map {map_name}: {e}")
        return False

def force_disk_commit(map_name, map_data):
    root = STUDIO["root_dir"]
    layouts_json = os.path.join(root, "data", "layouts", "layouts.json")

    if os.path.exists(layouts_json):
        try:
            with open(layouts_json, "r", encoding="utf-8") as f:
                idx = json.load(f)
            for item in idx.get("layouts", []):
                if item.get("id") == map_data["layout_id"]:
                    item["width"] = map_data["width"]
                    item["height"] = map_data["height"]
            with open(layouts_json, "w", encoding="utf-8") as f:
                json.dump(idx, f, indent=4)
        except Exception as e:
            print(f"[ERROR] Layout JSON config update failed: {e}")

    try:
        with open(map_data["map_bin_path"], "wb") as f:
            for entry in map_data["metatiles"]:
                f.write(int(entry).to_bytes(2, byteorder='little'))
        if os.path.exists(map_data["border_bin_path"]):
            with open(map_data["border_bin_path"], "wb") as f:
                for entry in map_data["border_blocks"]:
                    f.write(int(entry).to_bytes(2, byteorder='little'))
        print(f"[SUCCESS] Saved '{map_name}' files directly to decomp repository.")
        return True
    except Exception as e:
        print(f"[ERROR] IO Write breakdown on {map_name}: {e}")
        return False

class PoorymapWebBackend(BaseHTTPRequestHandler):
    def log_message(self, format, *args): 
        return

    def compile_tile(self, map_context, global_metatile_id, render_layer=2):
        m = STUDIO["maps"].get(map_context)
        if not m: 
            return None
        is_secondary = global_metatile_id >= STUDIO["secondary_offset"]

        if is_secondary:
            local_id = global_metatile_id - STUDIO["secondary_offset"]
            metatiles_bin = m["secondary_metatiles_bin"]
            tiles_png_path = m["secondary_tiles_png"]
            pals = STUDIO["palettes"]["secondary"].get(m["s_folder"], {})
        else:
            local_id = global_metatile_id
            metatiles_bin = m["primary_metatiles_bin"]
            tiles_png_path = m["primary_tiles_png"]
            pals = STUDIO["palettes"]["primary"].get(m["p_folder"], {})

        if not os.path.exists(metatiles_bin) or not os.path.exists(tiles_png_path):
            return Image.new("RGBA", (16, 16), (40, 40, 40, 255))

        try:
            with open(metatiles_bin, "rb") as f: 
                metatiles_buffer = f.read()
            src_png = Image.open(tiles_png_path).convert("P")
            tiles_per_row = src_png.width // 8

            p_png = Image.open(m["primary_tiles_png"])
            primary_tile_count = (p_png.width // 8) * (p_png.height // 8)

            canvas = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
            offset = local_id * 16
            if offset + 16 > len(metatiles_buffer): 
                return None

            grid_positions = [(0, 0), (8, 0), (0, 8), (8, 8)]
            for layer in range(2):
                # Determine layer ghosting states based on active layer selections
                ghost_layer = False
                if render_layer == 0 and layer == 1: # Only below is targeted, ghost above
                    ghost_layer = True
                elif render_layer == 1 and layer == 0: # Only above is targeted, ghost below
                    ghost_layer = True

                for i in range(4):
                    byte_offset = offset + (layer * 8) + (i * 2)
                    tile_value = int.from_bytes(metatiles_buffer[byte_offset:byte_offset+2], byteorder='little')

                    tile_id = tile_value & 0x03FF
                    h_flip = (tile_value >> 10) & 0x01
                    v_flip = (tile_value >> 11) & 0x01
                    palette_num = (tile_value >> 12) & 0x0F

                    if tile_id == 0 and layer == 1: 
                        continue
                    if is_secondary:
                        tile_id -= primary_tile_count
                        if tile_id < 0: 
                            tile_id = 0

                    s_row = tile_id // tiles_per_row
                    s_col = tile_id % tiles_per_row
                    tile_img_indexed = src_png.crop((s_col * 8, s_row * 8, (s_col + 1) * 8, (s_row + 1) * 8))

                    tile_rgba = tile_img_indexed.convert("RGBA")
                    pixels = tile_rgba.load()
                    active_pal = pals.get(palette_num, None)

                    if active_pal:
                        for y_px in range(8):
                            for x_px in range(8):
                                idx_color = tile_img_indexed.getpixel((x_px, y_px))
                                if idx_color % 16 == 0: 
                                    pixels[x_px, y_px] = (0, 0, 0, 0)
                                else:
                                    p_idx = idx_color % 16
                                    if p_idx < len(active_pal):
                                        r, g, b = active_pal[p_idx]
                                        alpha_val = 76 if ghost_layer else 255
                                        pixels[x_px, y_px] = (r, g, b, alpha_val)
                    elif ghost_layer:
                        # Apply fallback ghosting factor if pal entry layout fails
                        for y_px in range(8):
                            for x_px in range(8):
                                r, g, b, a = pixels[x_px, y_px]
                                if a > 0:
                                    pixels[x_px, y_px] = (r, g, b, 76)

                    if h_flip:
                        tile_rgba = tile_rgba.transpose(Image.FLIP_LEFT_RIGHT)
                    if v_flip:
                        tile_rgba = tile_rgba.transpose(Image.FLIP_TOP_BOTTOM)

                    canvas.alpha_composite(tile_rgba, grid_positions[i])
            return canvas
        except Exception: 
            return None

    def do_GET(self):
        if self.path.startswith("/?"): 
            self.path = "/"

        if self.path == "/":
            csv_path = os.path.join(os.getcwd(), "metatiles.csv")
            behavior_map = []
            if os.path.exists(csv_path):
                try:
                    with open(csv_path, mode="r", encoding="utf-8") as f:
                        for r in csv.DictReader(f):
                            behavior_map.append({"id": int(r.get("MetatileID", 0)), "behavior": r.get("BehaviorName", "UNKNOWN")})
                except Exception: 
                    pass

            html_template = """
            <!DOCTYPE html>
            <html>
            <head>
                <title>Pokeemerald Studio Environment</title>
                <style>
                    body { background-color: #0b0b0b; color: #00ff66; font-family: 'Courier New', monospace; margin: 0; padding: 15px; display: flex; gap: 15px; height: 98vh; box-sizing: border-box; }
                    .pane { background: #121212; border: 1px solid #00ff66; border-radius: 4px; padding: 12px; display: flex; flex-direction: column; overflow: hidden; }
                    #map-pane { flex: 2; }
                    #sidebar-pane { flex: 1; max-width: 440px; }
                    .grid-container { overflow: auto; background: #000; border: 1px solid #003311; border-radius: 2px; padding: 5px; flex-grow: 1; position: relative; }
                    .matrix { display: grid; gap: 1px; background-color: #051505; width: fit-content; }
                    .tile { width: 40px; height: 40px; background-color: #111; cursor: pointer; user-select: none; border: 1px solid #002208; display: flex; align-items: center; justify-content: center; box-sizing: border-box; position: relative; }
                    .tile img { width: 100%; height: 100%; image-rendering: pixelated; }
                    .tile:hover { border-color: #00ff66; z-index: 2; }
                    .tile.active { border-color: #ffffff !important; box-shadow: 0 0 6px #ffffff; z-index: 3; }
                    .tile.selected-range { background-color: #002244; border-color: #0088ff; opacity: 0.8; }
                    
                    /* Object/JSON Event Highlighting States */
                    .tile.evt-object { border: 2px solid #3399ff !important; box-shadow: inset 0 0 4px #3399ff; }
                    .tile.evt-warp { border: 2px solid #ff3333 !important; box-shadow: inset 0 0 4px #ff3333; }
                    .tile.evt-coord { border: 2px solid #ffcc00 !important; box-shadow: inset 0 0 4px #ffcc00; }
                    .tile.evt-bg { border: 2px solid #cc33ff !important; box-shadow: inset 0 0 4px #cc33ff; }
                    
                    .atlas-visual-matrix { display: grid; grid-template-columns: repeat(8, 1fr); gap: 4px; padding: 2px; overflow-y: auto; flex-grow: 1; }
                    .atlas-cell { background: #181818; border: 1px solid #002208; padding: 2px; text-align: center; cursor: pointer; box-sizing: border-box; position: relative; aspect-ratio: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; }
                    .atlas-cell img { width: 100%; height: auto; image-rendering: pixelated; max-height: 30px; object-fit: contain; }
                    .atlas-cell .cell-id-tag { position: absolute; bottom: 1px; right: 2px; font-size: 8px; color: #00ff66; background: rgba(0,0,0,0.7); padding: 0 2px; }
                    .atlas-cell:hover { border-color: #00ff66; }
                    .atlas-cell.tracker-highlight { border-color: #ff3333 !important; box-shadow: inset 0 0 4px #ff3333; background: #200505; }
                    .toolbar { display: flex; gap: 6px; margin-bottom: 8px; flex-wrap: wrap; background: #050505; padding: 6px; border: 1px solid #002208; border-radius: 2px; }
                    button { background: #001104; color: #00ff66; border: 1px solid #00ff66; padding: 4px 10px; border-radius: 2px; cursor: pointer; font-family: monospace; font-size: 11px; text-transform: uppercase; }
                    button:hover { background: #003311; color: #fff; }
                    button.active-toggle { background: #330000; border-color: #ff3333; color: #ff3333; }
                    .tabs { display: flex; gap: 4px; margin-bottom: -1px; z-index: 2; position: relative; overflow-x: auto; }
                    .tab { background: #1c1c1c; color: #888; border: 1px solid #003311; padding: 6px 12px; border-radius: 4px 4px 0 0; cursor: pointer; font-size: 11px; white-space: nowrap; }
                    .tab.active { background: #121212; color: #00ff66; border-bottom: 1px solid #121212; font-weight: bold; }
                    .meta-readout { background: #000; padding: 10px; border-radius: 2px; border: 1px solid #002208; font-size: 11px; margin-top: 8px; line-height: 1.5; color: #00ff66; overflow-y: auto; max-height: 300px; }
                    .section-title { font-weight: bold; color: #fff; border-bottom: 1px solid #002208; margin-bottom: 4px; padding-bottom: 2px; }
                    .property-row { display: flex; gap: 4px; margin-top: 6px; align-items: center; }
                    select, input[type="number"] { background: #000; color: #00ff66; border: 1px solid #00ff66; font-family: monospace; font-size: 11px; padding: 2px; border-radius: 2px; }
                </style>
            </head>
            <body>
                <div class="pane" id="map-pane">
                    <div class="tabs" id="map-tabs-container"></div>
                    <div class="toolbar">
                        <button onclick="saveCurrentMap()" style="font-weight: bold; border-color: #fff;" title="Ctrl+S">Save Studio</button>
                        <button id="btn-border" onclick="toggleBorderMode()" title="B">Border</button>
                        <button id="btn-select" onclick="toggleSelectionMode()" title="S">Select Range</button>
                        <button id="btn-hand" class="active-toggle" onclick="setTool('hand')" title="H">Hand (H)</button>
                        <button id="btn-draw" onclick="setTool('draw')" title="D">Draw (D)</button>
                        <button id="btn-picker" onclick="setTool('picker')" title="I">Eyedropper (I)</button>
                        <button onclick="copySelection()" title="C">Copy</button>
                        <button onclick="pasteSelection()" title="V">Paste</button>
                        <button onclick="promptOpenMap()">+ Load Tab</button>
                    </div>
                    
                    <div class="toolbar" style="background:#090909; border-color:#00441a;">
                        <span style="font-size:11px; align-self:center; color:#fff; margin-right:8px;">LAYERS:</span>
                        <button id="btn-layer-both" class="active-toggle" onclick="setLayerFilter(2)" title="Show Both Layers">Both</button>
                        <button id="btn-layer-below" onclick="setLayerFilter(0)" title="Focus Below Layer / Ghost Above">Below</button>
                        <button id="btn-layer-above" onclick="setLayerFilter(1)" title="Focus Above Layer / Ghost Below">Above</button>
                        
                        <span style="font-size:11px; align-self:center; color:#fff; margin-left:12px; margin-right:8px;">OVERLAYS:</span>
                        <button id="btn-toggle-events" onclick="toggleEventsOverlay()" title="Toggle JSON Map Objects Overlay (O)">Objects: OFF</button>
                    </div>

                    <div class="grid-container"><div id="map-matrix" class="matrix"></div></div>

                    <div class="toolbar" style="margin-top:8px; margin-bottom:0;">
                        <span style="font-size:11px; align-self:center; color:#fff; margin-right:8px;">SELECTION ATTRIBUTES:</span>
                        <div class="property-row" style="margin-top:0;">
                            <label>Elev:</label>
                            <input type="number" id="prop-elevation" min="0" max="15" value="0" onchange="applyPropertyField('elevation', this.value)">
                        </div>
                        <div class="property-row" style="margin-top:0; margin-left:8px;">
                            <label>Collision:</label>
                            <input type="number" id="prop-collision" min="0" max="3" value="0" onchange="applyPropertyField('collision', this.value)">
                        </div>
                        <div class="property-row" style="margin-top:0; margin-left:8px;">
                            <label>Bounds W:</label>
                            <input type="number" id="prop-width" min="1" max="128" style="width:45px;" onchange="resizeMapDimensions('width', this.value)">
                            <label>H:</label>
                            <input type="number" id="prop-height" min="1" max="128" style="width:45px;" onchange="resizeMapDimensions('height', this.value)">
                        </div>
                    </div>
                </div>

                <div class="pane" id="sidebar-pane">
                    <div class="toolbar">
                        <button id="btn-atlas-p" onclick="changeAtlasView('primary')" title="P">Primary Atlas</button>
                        <button id="btn-atlas-s" onclick="changeAtlasView('secondary')" title="O">Secondary Atlas</button>
                    </div>
                    <div class="grid-container" style="display: flex; flex-direction: column;"><div id="atlas-container" class="atlas-visual-matrix"></div></div>
                    <div class="meta-readout" id="readout-box">Select elements to initialize tracking properties.</div>
                </div>

                <script>
                    let STUDIO = __STUDIO_DATA__;
                    let BEHAVIORS = __BEHAVIOR_DATA__;
                    let activeMapName = Object.keys(STUDIO.maps)[0] || "";

                    let state = {
                        borderMode: false, selectionActive: false, selectionStart: null,
                        cursorIdx: 0, clipboard: null, atlasView: 'primary', currentTool: 'hand',
                        lastActiveTool: 'hand', selectedPaletteBlock: 0, layerFilter: 2,
                        showEvents: false // Toggles drawing custom event containers
                    };

                    function parseMetatile(val) {
                        return {
                            id: val & 0x03FF,
                            collision: (val >> 10) & 0x03,
                            elevation: (val >> 12) & 0x0F
                        };
                    }
                    function packMetatile(id, collision, elevation) {
                        return (id & 0x03FF) | ((collision & 0x03) << 10) | ((elevation & 0x0F) << 12);
                    }

                    function renderTabs() {
                        const container = document.getElementById("map-tabs-container");
                        container.innerHTML = "";
                        Object.keys(STUDIO.maps).forEach(name => {
                            let tab = document.createElement("div");
                            tab.className = "tab" + (name === activeMapName ? " active" : "");
                            tab.innerText = name;
                            tab.onclick = () => { switchMapTab(name); };
                            container.appendChild(tab);
                        });
                    }

                    function switchMapTab(name) {
                        activeMapName = name;
                        state.cursorIdx = 0;
                        state.selectionActive = false;
                        state.selectionStart = null;

                        let m = STUDIO.maps[activeMapName];
                        if(m) {
                            document.getElementById("prop-width").value = m.width;
                            document.getElementById("prop-height").value = m.height;
                        }

                        renderTabs();
                        renderMatrixGrid();
                        buildVisualAtlas();
                        updateReadout();
                    }

                    function setTool(toolName) {
                        if (state.currentTool !== 'picker') state.lastActiveTool = state.currentTool;
                        state.currentTool = toolName;
                        document.getElementById("btn-hand").classList.toggle("active-toggle", toolName === 'hand');
                        document.getElementById("btn-draw").classList.toggle("active-toggle", toolName === 'draw');
                        document.getElementById("btn-picker").classList.toggle("active-toggle", toolName === 'picker');
                    }

                    function setLayerFilter(layerValue) {
                        state.layerFilter = layerValue;
                        document.getElementById("btn-layer-both").classList.toggle("active-toggle", layerValue === 2);
                        document.getElementById("btn-layer-below").classList.toggle("active-toggle", layerValue === 0);
                        document.getElementById("btn-layer-above").classList.toggle("active-toggle", layerValue === 1);
                        renderMatrixGrid();
                        buildVisualAtlas();
                    }

                    function toggleEventsOverlay() {
                        state.showEvents = !state.showEvents;
                        let btn = document.getElementById("btn-toggle-events");
                        btn.classList.toggle("active-toggle", state.showEvents);
                        btn.innerText = "Objects: " + (state.showEvents ? "ON" : "OFF");
                        renderMatrixGrid();
                    }

                    function getSelectedIndices() {
                        let m = STUDIO.maps[activeMapName];
                        let width = state.borderMode ? 2 : m.width;
                        if (!state.selectionActive || state.selectionStart === null) return [state.cursorIdx];

                        let sX = state.selectionStart % width, sY = Math.floor(state.selectionStart / width);
                        let curX = state.cursorIdx % width, curY = Math.floor(state.cursorIdx / width);
                        let x1 = Math.min(sX, curX), x2 = Math.max(sX, curX);
                        let y1 = Math.min(sY, curY), y2 = Math.max(sY, curY);

                        let indices = [];
                        for (let y = y1; y <= y2; y++) {
                            for (let x = x1; x <= x2; x++) { indices.push(y * width + x); }
                        }
                        return indices;
                    }

                    function renderMatrixGrid() {
                        const grid = document.getElementById("map-matrix"); grid.innerHTML = "";
                        let m = STUDIO.maps[activeMapName]; if (!m) return;

                        let width = state.borderMode ? 2 : m.width;
                        let tiles = state.borderMode ? m.border_blocks : m.metatiles;
                        grid.style.gridTemplateColumns = `repeat(${width}, 40px)`;

                        tiles.forEach((entry, idx) => {
                            let meta = parseMetatile(entry);
                            let cell = document.createElement("div"); cell.className = "tile"; cell.id = `tile-${idx}`;
                            let img = document.createElement("img"); 
                            img.src = `/render_tile?map=${activeMapName}&id=${meta.id}&layer=${state.layerFilter}`;
                            cell.appendChild(img);

                            let cX = idx % width;
                            let cY = Math.floor(idx / width);

                            // Apply distinct color border tags based on JSON event arrays at specific grid space indices
                            if (state.showEvents && !state.borderMode) {
                                if (m.warp_events && m.warp_events.some(e => e.x === cX && e.y === cY)) {
                                    cell.classList.add("evt-warp");
                                } else if (m.object_events && m.object_events.some(e => e.x === cX && e.y === cY)) {
                                    cell.classList.add("evt-object");
                                } else if (m.coord_events && m.coord_events.some(e => e.x === cX && e.y === cY)) {
                                    cell.classList.add("evt-coord");
                                } else if (m.bg_events && m.bg_events.some(e => e.x === cX && e.y === cY)) {
                                    cell.classList.add("evt-bg");
                                }
                            }

                            if (idx === state.cursorIdx) cell.classList.add("active");
                            if (state.selectionActive && state.selectionStart !== null) {
                                let sX = state.selectionStart % width, sY = Math.floor(state.selectionStart / width);
                                let curX = state.cursorIdx % width, curY = Math.floor(state.cursorIdx / width);
                                if (cX >= Math.min(sX, curX) && cX <= Math.max(sX, curX) && cY >= Math.min(sY, curY) && cY <= Math.max(sY, curY)) cell.classList.add("selected-range");
                            }

                            cell.onclick = () => {
                                state.cursorIdx = idx;
                                if (state.currentTool === 'picker') {
                                    state.selectedPaletteBlock = meta.id;
                                    state.atlasView = (meta.id >= STUDIO.secondary_offset) ? 'secondary' : 'primary';
                                    setTool(state.lastActiveTool);
                                    buildVisualAtlas();
                                } else if (state.currentTool === 'draw') {
                                    applyTileToSelection(state.selectedPaletteBlock);
                                }
                                renderMatrixGrid(); updateReadout();
                            };
                            grid.appendChild(cell);
                        });
                    }

                    function buildVisualAtlas() {
                        const container = document.getElementById("atlas-container"); container.innerHTML = "";
                        let startId = state.atlasView === 'primary' ? 0 : STUDIO.secondary_offset;
                        let endId = startId + 512;

                        document.getElementById("btn-atlas-p").classList.toggle("active-toggle", state.atlasView === 'primary');
                        document.getElementById("btn-atlas-s").classList.toggle("active-toggle", state.atlasView === 'secondary');

                        for (let blockId = startId; blockId < endId; blockId++) {
                            let cell = document.createElement("div"); cell.className = "atlas-cell"; cell.id = `atlas-cell-${blockId}`;
                            let img = document.createElement("img"); 
                            img.src = `/render_tile?map=${activeMapName}&id=${blockId}&layer=${state.layerFilter}`; 
                            cell.appendChild(img);
                            if (blockId === state.selectedPaletteBlock) cell.classList.add("tracker-highlight");

                            cell.onclick = () => {
                                state.selectedPaletteBlock = blockId;
                                Array.from(container.children).forEach(c => c.classList.remove("tracker-highlight"));
                                cell.classList.add("tracker-highlight");

                                if (state.currentTool === 'draw') {
                                    applyTileToSelection(blockId);
                                }
                            };
                            container.appendChild(cell);
                        }
                    }

                    function applyTileToSelection(blockId) {
                        let m = STUDIO.maps[activeMapName];
                        let tiles = state.borderMode ? m.border_blocks : m.metatiles;
                        let targets = getSelectedIndices();

                        targets.forEach(idx => {
                            if (idx < tiles.length) {
                                let oldMeta = parseMetatile(tiles[idx]);
                                tiles[idx] = packMetatile(blockId, oldMeta.collision, oldMeta.elevation);
                            }
                        });
                        renderMatrixGrid(); updateReadout();
                    }

                    function applyPropertyField(field, val) {
                        let m = STUDIO.maps[activeMapName];
                        let tiles = state.borderMode ? m.border_blocks : m.metatiles;
                        let targets = getSelectedIndices();
                        let intVal = parseInt(val) || 0;

                        targets.forEach(idx => {
                            if (idx < tiles.length) {
                                let meta = parseMetatile(tiles[idx]);
                                meta[field] = intVal;
                                tiles[idx] = packMetatile(meta.id, meta.collision, meta.elevation);
                            }
                        });
                        renderMatrixGrid(); updateReadout();
                    }

                    function changeAtlasView(type) { state.atlasView = type; buildVisualAtlas(); updateReadout(); }

                    function resizeMapDimensions(dim, val) {
                        if (state.borderMode) return;
                        let m = STUDIO.maps[activeMapName];
                        let newV = parseInt(val) || 20;
                        let oldW = m.width, oldH = m.height;
                        let newW = (dim === 'width') ? newV : oldW;
                        let newH = (dim === 'height') ? newV : oldH;

                        let newTiles = new Array(newW * newH).fill(0);
                        for (let y = 0; y < Math.min(oldH, newH); y++) {
                            for (let x = 0; x < Math.min(oldW, newW); x++) {
                                newTiles[y * newW + x] = m.metatiles[y * oldW + x];
                            }
                        }
                        m.width = newW; m.height = newH; m.metatiles = newTiles;
                        renderMatrixGrid();
                    }

                    function updateReadout() {
                        let m = STUDIO.maps[activeMapName];
                        let width = state.borderMode ? 2 : m.width;
                        let tiles = state.borderMode ? m.border_blocks : m.metatiles;
                        let meta = parseMetatile(tiles[state.cursorIdx] || 0);
                        let match = BEHAVIORS.find(b => b.id === meta.id);

                        document.getElementById("prop-elevation").value = meta.elevation;
                        document.getElementById("prop-collision").value = meta.collision;

                        let activeLayerStr = "BOTH";
                        if(state.layerFilter === 0) activeLayerStr = "BELOW (GHOST ABOVE)";
                        if(state.layerFilter === 1) activeLayerStr = "ABOVE (GHOST BELOW)";

                        let cX = state.cursorIdx % width;
                        let cY = Math.floor(state.cursorIdx / width);

                        let outputHtml = `
                            <div class="section-title">ELEMENT CONFIGURATION</div>
                            <strong>Current Target:</strong> ${activeMapName}<br>
                            <strong>Tile Position:</strong> X: ${cX}, Y: ${cY}<br>
                            <strong>Global Metatile ID:</strong> ${meta.id}<br>
                            <strong>Collision Bits:</strong> ${meta.collision}<br>
                            <strong>Elevation Level:</strong> ${meta.elevation}<br>
                            <strong>Active Tool:</strong> ${state.currentTool.toUpperCase()}<br>
                            <strong>Active Layer:</strong> ${activeLayerStr}<br>
                            <strong>Behavior Match:</strong> ${match ? match.behavior : "UNKNOWN"}
                        `;

                        // Evaluate structured data match arrays if overlay tracking state is active
                        if (state.showEvents && !state.borderMode) {
                            let matchObj = [];
                            if (m.warp_events) m.warp_events.forEach(e => { if (e.x === cX && e.y === cY) matchObj.push({type: "WARP EVENT", data: e}); });
                            if (m.object_events) m.object_events.forEach(e => { if (e.x === cX && e.y === cY) matchObj.push({type: "OBJECT EVENT", data: e}); });
                            if (m.coord_events) m.coord_events.forEach(e => { if (e.x === cX && e.y === cY) matchObj.push({type: "COORD EVENT", data: e}); });
                            if (m.bg_events) m.bg_events.forEach(e => { if (e.x === cX && e.y === cY) matchObj.push({type: "BG / SIGN EVENT", data: e}); });

                            if (matchObj.length > 0) {
                                outputHtml += `<div class="section-title" style="margin-top:12px;">JSON OBJECT MANIFEST (${matchObj.length})</div>`;
                                matchObj.forEach(obj => {
                                    outputHtml += `
                                        <span style="color:#fff; font-weight:bold;">[${obj.type}]</span>
                                        <pre style="margin:4px 0 8px 0; background:#050505; border:1px solid #003311; padding:6px; color:#00ff88; font-size:10px;">${JSON.stringify(obj.data, null, 2)}</pre>
                                    `;
                                });
                            }
                        }

                        document.getElementById("readout-box").innerHTML = outputHtml;
                    }

                    function toggleBorderMode() {
                        state.borderMode = !state.borderMode; state.cursorIdx = 0; state.selectionActive = false;
                        document.getElementById("btn-border").classList.toggle("active-toggle", state.borderMode);
                        renderMatrixGrid(); updateReadout();
                    }
                    function toggleSelectionMode() { state.selectionActive = !state.selectionActive; state.selectionStart = state.selectionActive ? state.cursorIdx : null; document.getElementById("btn-select").classList.toggle("active-toggle", state.selectionActive); renderMatrixGrid(); }

                    function copySelection() {
                        if (!state.selectionActive || state.selectionStart === null) return;
                        let m = STUDIO.maps[activeMapName];
                        let width = state.borderMode ? 2 : m.width;
                        let sX = state.selectionStart % width, sY = Math.floor(state.selectionStart / width);
                        let curX = state.cursorIdx % width, curY = Math.floor(state.cursorIdx / width);
                        let x1 = Math.min(sX, curX), x2 = Math.max(sX, curX), y1 = Math.min(sY, curY), y2 = Math.max(sY, curY);
                        let tiles = state.borderMode ? m.border_blocks : m.metatiles;

                        state.clipboard = { w: x2 - x1 + 1, h: y2 - y1 + 1, blocks: [] };
                        for(let y=y1; y<=y2; y++) { for(let x=x1; x<=x2; x++) { state.clipboard.blocks.push(tiles[y * width + x]); } }
                        state.selectionActive = false; document.getElementById("btn-select").classList.remove("active-toggle"); renderMatrixGrid();
                    }

                    function pasteSelection() {
                        if (!state.clipboard) return;
                        let m = STUDIO.maps[activeMapName];
                        let width = state.borderMode ? 2 : m.width;
                        let height = Math.ceil((state.borderMode ? m.border_blocks : m.metatiles).length / width);
                        let tiles = state.borderMode ? m.border_blocks : m.metatiles;
                        let startX = state.cursorIdx % width, startY = Math.floor(state.cursorIdx / width);

                        for(let y=0; y<state.clipboard.h; y++) {
                            if (startY + y >= height) break;
                            for(let x=0; x<state.clipboard.w; x++) { if (startX + x >= width) break; tiles[(startY + y) * width + (startX + x)] = state.clipboard.blocks[y * state.clipboard.w + x]; }
                        }
                        renderMatrixGrid(); updateReadout();
                    }

                    function promptOpenMap() {
                        let name = prompt("Enter map directory name inside 'data/maps/':");
                        if (name) {
                            fetch(`/open_map?name=${name}`).then(res => res.json()).then(data => {
                                if (data.status === "success") {
                                    STUDIO.maps[name] = data.payload;
                                    switchMapTab(name);
                                } else alert("Failed to stage target folder structure.");
                            });
                        }
                    }

                    function saveCurrentMap() {
                        let m = STUDIO.maps[activeMapName];
                        fetch('/save', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ map_name: activeMapName, width: m.width, height: m.height, metatiles: m.metatiles, border_blocks: m.border_blocks }) })
                        .then(res => res.json()).then(data => alert(data.message));
                    }

                    window.addEventListener("keydown", (e) => {
                        let m = STUDIO.maps[activeMapName]; if(!m) return;
                        let width = state.borderMode ? 2 : m.width;
                        let maxLen = (state.borderMode ? m.border_blocks : m.metatiles).length;

                        if (e.key === "ArrowUp" && state.cursorIdx >= width) state.cursorIdx -= width;
                        else if (e.key === "ArrowDown" && state.cursorIdx + width < maxLen) state.cursorIdx += width;
                        else if (e.key === "ArrowLeft" && state.cursorIdx % width > 0) state.cursorIdx -= 1;
                        else if (e.key === "ArrowRight" && state.cursorIdx % width < width - 1) state.cursorIdx += 1;
                        else if (e.key.toLowerCase() === "s" && e.ctrlKey) { e.preventDefault(); saveCurrentMap(); return; }
                        else if (e.key.toLowerCase() === "s") { toggleSelectionMode(); return; }
                        else if (e.key.toLowerCase() === "c") { copySelection(); return; }
                        else if (e.key.toLowerCase() === "v") { pasteSelection(); return; }
                        else if (e.key.toLowerCase() === "b") { toggleBorderMode(); return; }
                        else if (e.key.toLowerCase() === "h") { setTool('hand'); return; }
                        else if (e.key.toLowerCase() === "d") { setTool('draw'); return; }
                        else if (e.key.toLowerCase() === "i") { setTool('picker'); return; }
                        else if (e.key.toLowerCase() === "p") { changeAtlasView('primary'); return; }
                        else if (e.key.toLowerCase() === "o") { toggleEventsOverlay(); return; }
                        else return;
                        renderMatrixGrid(); updateReadout();
                    });

                    switchMapTab(activeMapName);
                </script>
            </body>
            </html>
            """.replace("__STUDIO_DATA__", json.dumps(STUDIO)).replace("__BEHAVIOR_DATA__", json.dumps(behavior_map))

            encoded_html = html_template.encode('utf-8')

            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(encoded_html)))
            self.end_headers()
            self.wfile.write(encoded_html)

        elif self.path.startswith("/render_tile"):
            params = re.findall(r'map=([^&]+)', self.path)
            map_ctx = params[0] if params else ""
            id_params = re.findall(r'id=(\d+)', self.path)
            global_id = int(id_params[0]) if id_params else 0
            
            # Read layer parameter from query
            layer_params = re.findall(r'layer=(\d+)', self.path)
            render_layer = int(layer_params[0]) if layer_params else 2

            tile_img = self.compile_tile(map_ctx, global_id, render_layer)
            if tile_img:
                buf = io.BytesIO()
                tile_img.save(buf, format="PNG")
                img_data = buf.getvalue()

                self.send_response(200)
                self.send_header("Content-Type", "image/png")
                self.send_header("Content-Length", str(len(img_data)))
                self.end_headers()
                self.wfile.write(img_data)
            else:
                self.send_response(404)
                self.end_headers()

        elif self.path.startswith("/open_map"):
            name_param = re.findall(r'name=([^&]+)', self.path)
            target_map = name_param[0] if name_param else ""
            success = stage_map_into_studio(STUDIO["root_dir"], target_map)

            payload_data = json.dumps({"status": "success", "payload": STUDIO["maps"][target_map] if success else None}).encode('utf-8')

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload_data)))
            self.end_headers()
            self.wfile.write(payload_data)

    def do_POST(self):
        if self.path == "/save":
            content_length = int(self.headers['Content-Length'])
            data = json.loads(self.rfile.read(content_length).decode('utf-8'))
            name = data.get("map_name")

            if name in STUDIO["maps"]:
                m = STUDIO["maps"][name]
                m["width"] = data.get("width", m["width"])
                m["height"] = data.get("height", m["height"])
                m["metatiles"] = data.get("metatiles", m["metatiles"])
                m["border_blocks"] = data.get("border_blocks", m["border_blocks"])

                if force_disk_commit(name, m):
                    res_msg = json.dumps({"message": f"Successfully committed updates for '{name}' straight to file system paths."}).encode('utf-8')
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.send_header("Content-Length", str(len(res_msg)))
                    self.end_headers()
                    self.wfile.write(res_msg)
                    return
            self.send_response(500)
            self.end_headers()

def main():
    parser = argparse.ArgumentParser(description="Pokeemerald Unified Workspace Framework")
    parser.add_argument("root_dir", help="Path to pokeemerald repository root directory")
    parser.add_argument("map_name", help="Default entry map directory identifier to instantiate")
    parser.add_argument("--delete", "-d", action="store_true", help="Delete the map files and configurations instead of creating or staging them")
    args = parser.parse_args()

    STUDIO["root_dir"] = args.root_dir

    if args.delete:
        delete_map_structure(args.root_dir, args.map_name)
        print(f"[SUCCESS] Complete map purge finalized for: '{args.map_name}'")
        sys.exit(0)

    stage_map_into_studio(args.root_dir, args.map_name)

    server = ThreadingHTTPServer(('0.0.0.0', 8080), PoorymapWebBackend)
    print(f"Workspace studio engine active: http://localhost:8080")
    try: 
        server.serve_forever()
    except KeyboardInterrupt: 
        print("\nClean termination.")

if __name__ == "__main__":
    main()
