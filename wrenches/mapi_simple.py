#!/usr/bin/env python3
import sys
import os
import re
import io
import json
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    from PIL import Image
except ImportError:
    print("[CRITICAL] 'Pillow' library is required. Run: pip install Pillow")
    sys.exit(1)

STUDIO = {
    "root_dir": "",
    "secondary_offset": 512,
    "palettes": {"primary": {}},
    "maps": {}
}

def load_gba_pal_file(filepath):
    """
    Accepts a simple text file where each line is: R G B
    Returns a list of (r,g,b) tuples if >= 16 entries present else None.
    """
    colors = []
    if not filepath or not os.path.exists(filepath):
        return None
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                m = re.match(r'^(\d+)\s+(\d+)\s+(\d+)$', line)
                if m:
                    colors.append((int(m.group(1)), int(m.group(2)), int(m.group(3))))
        return colors if len(colors) >= 16 else None
    except Exception:
        return None

def read_metatiles_bin(bin_path):
    """
    Reads a metatiles.bin-like file where each metatile entry is 16 bytes (4 tiles * 2 layers * 2 bytes).
    We'll read as sequence of 2-byte little endian words (the original code reads per 2 bytes).
    """
    metatiles = []
    if not os.path.exists(bin_path):
        return metatiles
    try:
        with open(bin_path, "rb") as f:
            while (b := f.read(2)):
                metatiles.append(int.from_bytes(b, byteorder='little'))
    except Exception as e:
        print(f"[ERROR] Failed reading metatiles bin: {e}")
    return metatiles

def stage_single_map(map_name, png_path, metatiles_bin_path, pal_path, width):
    """
    Populate STUDIO with one map from provided files.
    - png_path: tiles.png (atlas)
    - metatiles_bin_path: the binary file (2-byte words per entry)
    - pal_path: optional palette text file
    - width: map width in tiles (metatiles per row)
    """
    if not os.path.exists(png_path):
        print(f"[ERROR] Tiles PNG not found: {png_path}")
        return False
    if not os.path.exists(metatiles_bin_path):
        print(f"[ERROR] Metatiles bin not found: {metatiles_bin_path}")
        return False

    metatiles = read_metatiles_bin(metatiles_bin_path)
    if not metatiles:
        print("[WARN] Metatiles list is empty.")

    height = (len(metatiles) + max(1, width) - 1) // max(1, width)

    # primary palette mapping: map palette 0 -> provided palette (if any)
    pal_colors = load_gba_pal_file(pal_path) if pal_path else None
    # store under a stable key
    folder_key = "single"
    STUDIO["palettes"]["primary"][folder_key] = {}
    if pal_colors:
        # map palette number 0 to the provided palette
        STUDIO["palettes"]["primary"][folder_key][0] = pal_colors

    STUDIO["maps"][map_name] = {
        "map_name": map_name,
        "layout_id": f"LAYOUT_{map_name.upper()}",
        "width": width,
        "height": height,
        "p_folder": folder_key,
        "primary_tiles_png": png_path,
        "primary_metatiles_bin": metatiles_bin_path,
        "metatiles": metatiles,
        "border_blocks": [],  # not used by this simplified loader
        "object_events": [],
        "warp_events": [],
        "coord_events": [],
        "bg_events": [],
        "map_headers": {}
    }
    return True

class SimpleMapBackend(BaseHTTPRequestHandler):
    def log_message(self, format, *args): 
        return

    def compile_tile(self, map_context, global_metatile_id, render_layer=2):
        m = STUDIO["maps"].get(map_context)
        if not m:
            return None

        # Only primary supported in this simplified version
        is_secondary = False

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
            tiles_per_row = max(1, src_png.width // 8)

            canvas = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
            offset = local_id * 16
            if offset + 16 > len(metatiles_buffer):
                return None

            grid_positions = [(0, 0), (8, 0), (0, 8), (8, 8)]
            # There are 2 layers (below, above), each layer has four 2-byte tile entries (8 bytes) => 16 bytes per metatile
            for layer in range(2):
                ghost_layer = False
                if render_layer == 0 and layer == 1:
                    ghost_layer = True
                elif render_layer == 1 and layer == 0:
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

                    s_row = tile_id // tiles_per_row
                    s_col = tile_id % tiles_per_row
                    # if tile index is out of the image bounds, return blank tile for that sub-tile
                    if (s_col * 8 + 8 > src_png.width) or (s_row * 8 + 8 > src_png.height):
                        tile_rgba = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
                    else:
                        tile_img_indexed = src_png.crop((s_col * 8, s_row * 8, (s_col + 1) * 8, (s_row + 1) * 8))
                        tile_rgba = tile_img_indexed.convert("RGBA")
                        pixels = tile_rgba.load()
                        active_pal = pals.get(palette_num, None)

                        if active_pal:
                            for y_px in range(8):
                                for x_px in range(8):
                                    idx_color = tile_img_indexed.getpixel((x_px, y_px))
                                    # transparent index handling
                                    if idx_color % 16 == 0:
                                        pixels[x_px, y_px] = (0, 0, 0, 0)
                                    else:
                                        p_idx = idx_color % 16
                                        if p_idx < len(active_pal):
                                            r, g, b = active_pal[p_idx]
                                            alpha_val = 76 if ghost_layer else 255
                                            pixels[x_px, y_px] = (r, g, b, alpha_val)
                        elif ghost_layer:
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
        except Exception as e:
            # print("compile error", e)
            return None

    def do_GET(self):
        if self.path == "/" or self.path.startswith("/?"):
            # build a compact UI that renders the grid and the atlas
            behavior_map = []
            csv_path = os.path.join(os.getcwd(), "metatiles.csv")
            if os.path.exists(csv_path):
                try:
                    import csv
                    with open(csv_path, "r", encoding="utf-8") as f:
                        for r in csv.DictReader(f):
                            behavior_map.append({"id": int(r.get("MetatileID", 0)), "behavior": r.get("BehaviorName", "UNKNOWN")})
                except Exception:
                    pass

            html = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Simple Map Studio</title>
<style>
body {{ background:#0b0b0b; color:#00ff66; font-family:monospace; margin:0; padding:12px; }}
.container {{ display:flex; gap:12px; }}
.pane {{ background:#111; border:1px solid #003311; padding:8px; border-radius:4px; }}
.matrix {{ display:grid; gap:2px; background:#050505; padding:4px; }}
.tile {{ width:32px; height:32px; border:1px solid #002208; box-sizing:border-box; background:#000; }}
.tile img {{ width:100%; height:100%; image-rendering:pixelated; }}
.atlas {{ display:grid; grid-template-columns:repeat(8,1fr); gap:4px; max-height:400px; overflow:auto; }}
.atlas-cell {{ border:1px solid #002208; padding:2px; background:#111; cursor:pointer; }}
</style>
</head>
<body>
<div style="margin-bottom:8px;">
<button onclick="saveCurrentMap()">Save</button>
Map: <strong>{list(STUDIO['maps'].keys())[0] if STUDIO['maps'] else ''}</strong>
</div>
<div class="container">
  <div class="pane" id="map-pane">
    <div id="map-matrix" class="matrix"></div>
  </div>
  <div class="pane" id="atlas-pane" style="width:260px;">
    <div class="atlas" id="atlas-container"></div>
  </div>
</div>

<script>
let STUDIO = {json.dumps(STUDIO)};
let BEHAVIORS = {json.dumps(behavior_map)};
let activeMap = Object.keys(STUDIO.maps)[0];
let state = {{ selectedBlock:0, tool:'draw' }};

function parseMetatile(val) {{
    return {{
        id: val & 0x03FF,
        collision: (val >> 10) & 0x03,
        elevation: (val >> 12) & 0x0F
    }};
}}
function packMetatile(id, collision, elevation) {{
    return (id & 0x03FF) | ((collision & 0x03) << 10) | ((elevation & 0x0F) << 12);
}}

function renderMatrix() {{
    let m = STUDIO.maps[activeMap];
    if(!m) return;
    let width = m.width;
    let height = Math.ceil(m.metatiles.length / width);
    let container = document.getElementById('map-matrix');
    container.style.gridTemplateColumns = 'repeat(' + width + ', 32px)';
    container.innerHTML = '';
    for (let y=0;y<height;y++) {{
        for (let x=0;x<width;x++) {{
            let idx = y*width + x;
            let div = document.createElement('div');
            div.className='tile';
            div.id='tile-'+idx;
            let img = document.createElement('img');
            img.src = '/render_tile?map=' + encodeURIComponent(activeMap) + '&id=' + (m.metatiles[idx] ? (m.metatiles[idx] & 0x03FF) : 0) + '&layer=2';
            div.appendChild(img);
            div.onclick = () => {{
                if (state.tool==='draw') {{
                    let old = m.metatiles[idx] || 0;
                    let meta = parseMetatile(old);
                    m.metatiles[idx] = packMetatile(state.selectedBlock, meta.collision, meta.elevation);
                    renderMatrix();
                }} else if (state.tool === 'picker') {{
                    let v = m.metatiles[idx] || 0;
                    state.selectedBlock = parseMetatile(v).id;
                    buildAtlas();
                }}
            }};
            container.appendChild(div);
        }}
    }}
}}

function buildAtlas() {{
    let container = document.getElementById('atlas-container');
    container.innerHTML = '';
    let start = 0;
    let end = STUDIO.secondary_offset; // show 512 blocks
    for(let i=start;i<end;i++) {{
        let c = document.createElement('div'); c.className='atlas-cell';
        let img = document.createElement('img');
        img.src = '/render_tile?map=' + encodeURIComponent(activeMap) + '&id=' + i + '&layer=2';
        img.style.width='100%';
        c.appendChild(img);
        let tag = document.createElement('div'); tag.style.fontSize='9px'; tag.style.color='#00ff66'; tag.innerText = i;
        c.appendChild(tag);
        c.onclick = () => {{
            state.selectedBlock = i;
        }};
        if (i===state.selectedBlock) c.style.boxShadow='0 0 6px #fff';
        container.appendChild(c);
    }}
}}

function saveCurrentMap() {{
    let m = STUDIO.maps[activeMap];
    fetch('/save', {{
        method:'POST',
        headers:{{'Content-Type':'application/json'}},
        body: JSON.stringify({{ map_name: activeMap, metatiles: m.metatiles }})
    }}).then(r=>r.json()).then(j=>alert(j.message || JSON.stringify(j)));
}}

renderMatrix();
buildAtlas();
</script>
</body>
</html>
"""
            data = html.encode('utf-8')
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return

        elif self.path.startswith("/render_tile"):
            params = re.findall(r'map=([^&]+)', self.path)
            map_ctx = params[0] if params else ""
            id_params = re.findall(r'id=(\d+)', self.path)
            global_id = int(id_params[0]) if id_params else 0
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
                return
            else:
                self.send_response(404)
                self.end_headers()
                return

        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        if self.path == "/save":
            length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(length).decode('utf-8')
            try:
                data = json.loads(body)
                name = data.get("map_name")
                if not name or name not in STUDIO["maps"]:
                    self.send_response(400)
                    self.end_headers()
                    return
                m = STUDIO["maps"][name]
                metatiles = data.get("metatiles")
                if isinstance(metatiles, list):
                    m["metatiles"] = metatiles
                    # Write back to bin file as 2-byte little endian words
                    try:
                        with open(m["primary_metatiles_bin"], "wb") as f:
                            for entry in m["metatiles"]:
                                f.write(int(entry).to_bytes(2, byteorder='little'))
                        resp = {"message": f"Saved metatiles to {m['primary_metatiles_bin']}"}
                        payload = json.dumps(resp).encode('utf-8')
                        self.send_response(200)
                        self.send_header("Content-Type", "application/json")
                        self.send_header("Content-Length", str(len(payload)))
                        self.end_headers()
                        self.wfile.write(payload)
                        return
                    except Exception as e:
                        print("Save error:", e)
                self.send_response(500)
                self.end_headers()
            except Exception:
                self.send_response(400)
                self.end_headers()

def main():
    parser = argparse.ArgumentParser(description="Simple single-bin Map Studio")
    parser.add_argument("--png", required=True, help="Tiles PNG (atlas) path")
    parser.add_argument("--bin", required=True, help="Metatiles .bin path (2-byte words)")
    parser.add_argument("--pal", required=False, help="Optional palette file (text lines 'R G B')")
    parser.add_argument("--width", required=True, type=int, help="Map width in tiles (metatiles per row)")
    parser.add_argument("--map-name", default="singlemap", help="Map name to use in studio")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()

    STUDIO["root_dir"] = os.getcwd()
    ok = stage_single_map(args.map_name, args.png, args.bin, args.pal, args.width)
    if not ok:
        print("[ERROR] Failed to stage single map. Exiting.")
        sys.exit(1)

    server = ThreadingHTTPServer(('0.0.0.0', args.port), SimpleMapBackend)
    print(f"Simple map studio active: http://localhost:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")

if __name__ == "__main__":
    main()