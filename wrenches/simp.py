import os
import sys
import re
import json
import argparse
import io
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    from PIL import Image
except ImportError:
    print("[CRITICAL] 'Pillow' library is required. Run: pip install Pillow")
    sys.exit(1)

# Global storage for single-map editing session
WORKSPACE = {
    "bin_path": "",
    "png_path": "",
    "pal_path": "",
    "width": 20,
    "metatiles": [],
    "palette": [],
    "total_tiles_in_png": 0
}

def load_gba_pal_file(filepath):
    colors = []
    if not os.path.exists(filepath):
        return colors
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            for line in f:
                match = re.match(r'^(\d+)\s+(\d+)\s+(\d+)$', line.strip())
                if match:
                    colors.append((int(match.group(1)), int(match.group(2)), int(match.group(3))))
        return colors
    except Exception as e:
        print(f"[ERROR] Could not parse PAL file: {e}")
        return colors

def compile_metatile_image(metatile_val):
    tile_id = metatile_val & 0x03FF
    h_flip = (metatile_val >> 10) & 0x01
    v_flip = (metatile_val >> 11) & 0x01
    
    if not os.path.exists(WORKSPACE["png_path"]):
        return Image.new("RGBA", (8, 8), (40, 40, 40, 255))

    try:
        src_png = Image.open(WORKSPACE["png_path"]).convert("P")
        tiles_per_row = max(1, src_png.width // 8)

        s_row = tile_id // tiles_per_row
        s_col = tile_id % tiles_per_row

        tile_img = src_png.crop((s_col * 8, s_row * 8, (s_col + 1) * 8, (s_row + 1) * 8))
        tile_rgba = tile_img.convert("RGBA")
        pixels = tile_rgba.load()

        pal = WORKSPACE["palette"]
        if pal:
            for y in range(8):
                for x in range(8):
                    idx_color = tile_img.getpixel((x, y))
                    if idx_color % 16 == 0:
                        pixels[x, y] = (0, 0, 0, 0)
                    else:
                        p_idx = idx_color % len(pal)
                        r, g, b = pal[p_idx]
                        pixels[x, y] = (r, g, b, 255)

        if h_flip:
            tile_rgba = tile_rgba.transpose(Image.FLIP_LEFT_RIGHT)
        if v_flip:
            tile_rgba = tile_rgba.transpose(Image.FLIP_TOP_BOTTOM)

        return tile_rgba
    except Exception:
        return Image.new("RGBA", (8, 8), (255, 0, 0, 255))

class StandaloneMapBackend(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        return

    def do_GET(self):
        if self.path.startswith("/render_tile"):
            id_match = re.search(r'id=(\d+)', self.path)
            tile_id = int(id_match.group(1)) if id_match else 0
            
            img = compile_metatile_image(tile_id)
            buf = io.BytesIO()
            img.save(buf, format="PNG")
            img_bytes = buf.getvalue()

            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(img_bytes)))
            self.end_headers()
            self.wfile.write(img_bytes)
            return

        html = """<!DOCTYPE html>
<html>
<head>
    <title>Binary Tilemap Workspace</title>
    <style>
        body { background: #111; color: #00ff66; font-family: monospace; margin: 0; padding: 16px; display: flex; gap: 16px; height: 95vh; box-sizing: border-box; }
        .panel { background: #1a1a1a; border: 1px solid #00ff66; border-radius: 4px; padding: 12px; display: flex; flex-direction: column; overflow: hidden; }
        #map-container { flex: 2; overflow: auto; }
        #sidebar { flex: 1; max-width: 360px; display: flex; flex-direction: column; gap: 12px; }
        
        .grid { display: grid; gap: 1px; background: #222; width: fit-content; border: 1px solid #00ff66; }
        .tile { width: 32px; height: 32px; background: #000; cursor: pointer; display: flex; align-items: center; justify-content: center; position: relative; }
        .tile img { width: 100%; height: 100%; image-rendering: pixelated; }
        .tile.selected { border: 1px solid #fff; box-sizing: border-box; }
        
        #drawer-container { flex: 1; overflow-y: auto; background: #0b0b0b; border: 1px solid #00ff66; padding: 6px; }
        .drawer-grid { display: grid; grid-template-columns: repeat(8, 32px); gap: 2px; }
        .drawer-tile { width: 32px; height: 32px; background: #181818; cursor: pointer; border: 1px solid #222; box-sizing: border-box; }
        .drawer-tile img { width: 100%; height: 100%; image-rendering: pixelated; }
        .drawer-tile.active-draw { border-color: #ff3333; box-shadow: 0 0 6px #ff3333; }
        
        .slider-row { display: flex; flex-direction: column; gap: 4px; }
        input[type="range"] { accent-color: #00ff66; width: 100%; }
        button { background: #002200; color: #00ff66; border: 1px solid #00ff66; padding: 8px; cursor: pointer; font-family: monospace; font-weight: bold; }
        button:hover { background: #004400; }
        .toolbar { display: flex; gap: 6px; }
        .tool-btn { flex: 1; text-align: center; font-size: 11px; }
        .tool-btn.active { background: #00ff66; color: #000; }
    </style>
</head>
<body>
    <div class="panel" id="map-container">
        <div id="grid" class="grid"></div>
    </div>

    <div class="panel" id="sidebar">
        <h3>TILE DRAWER</h3>
        <div class="toolbar">
            <button id="btn-draw" class="tool-btn active" onclick="setMode('draw')">DRAW</button>
            <button id="btn-pick" class="tool-btn" onclick="setMode('pick')">EYEDROPPER</button>
        </div>

        <div id="drawer-container">
            <div id="drawer-grid" class="drawer-grid"></div>
        </div>

        <div class="slider-row">
            <label>Width: <span id="width-val">20</span> tiles</label>
            <input type="range" id="width-slider" min="1" max="64" value="20" oninput="updateWidth(this.value)">
        </div>

        <div>
            <p>Active Tile ID: <span id="active-id">0x0000</span></p>
            <p>Selected Index: <span id="sel-idx">-</span></p>
        </div>

        <button onclick="saveBin()">Save to .BIN</button>
    </div>

    <script>
        let DATA = __WORKSPACE_DATA__;
        let activeTileId = 0;
        let selectedIdx = 0;
        let mode = 'draw';

        function setMode(m) {
            mode = m;
            document.getElementById("btn-draw").classList.toggle("active", mode === 'draw');
            document.getElementById("btn-pick").classList.toggle("active", mode === 'pick');
        }

        function renderGrid() {
            const grid = document.getElementById("grid");
            grid.innerHTML = "";
            grid.style.gridTemplateColumns = `repeat(${DATA.width}, 32px)`;

            document.getElementById("width-val").innerText = DATA.width;

            DATA.metatiles.forEach((val, idx) => {
                let cell = document.createElement("div");
                cell.className = "tile" + (idx === selectedIdx ? " selected" : "");
                let img = document.createElement("img");
                img.src = `/render_tile?id=${val}`;
                cell.appendChild(img);

                cell.onclick = () => {
                    selectedIdx = idx;
                    document.getElementById("sel-idx").innerText = idx;

                    if (mode === 'draw') {
                        DATA.metatiles[idx] = activeTileId;
                        img.src = `/render_tile?id=${activeTileId}`;
                    } else if (mode === 'pick') {
                        activeTileId = val & 0x03FF;
                        updateActiveTileDisplay();
                        setMode('draw');
                    }
                    renderGrid();
                };

                grid.appendChild(cell);
            });
        }

        function renderDrawer() {
            const drawer = document.getElementById("drawer-grid");
            drawer.innerHTML = "";

            for (let id = 0; id < DATA.total_tiles_in_png; id++) {
                let cell = document.createElement("div");
                cell.className = "drawer-tile" + (id === activeTileId ? " active-draw" : "");
                let img = document.createElement("img");
                img.src = `/render_tile?id=${id}`;
                cell.appendChild(img);

                cell.onclick = () => {
                    activeTileId = id;
                    updateActiveTileDisplay();
                    renderDrawer();
                };

                drawer.appendChild(cell);
            }
        }

        function updateActiveTileDisplay() {
            document.getElementById("active-id").innerText = "0x" + activeTileId.toString(16).padStart(4, '0').toUpperCase();
        }

        function updateWidth(val) {
            DATA.width = parseInt(val);
            renderGrid();
        }

        function saveBin() {
            fetch('/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ width: DATA.width, metatiles: DATA.metatiles })
            })
            .then(res => res.json())
            .then(d => alert(d.message));
        }

        document.getElementById("width-slider").value = DATA.width;
        updateActiveTileDisplay();
        renderDrawer();
        renderGrid();
    </script>
</body>
</html>""".replace("__WORKSPACE_DATA__", json.dumps(WORKSPACE))

        encoded_html = html.encode('utf-8')
        self.send_response(200)
        self.send_header("Content-Type", "text/html")
        self.send_header("Content-Length", str(len(encoded_html)))
        self.end_headers()
        self.wfile.write(encoded_html)

    def do_POST(self):
        if self.path == "/save":
            length = int(self.headers['Content-Length'])
            payload = json.loads(self.rfile.read(length).decode('utf-8'))
            
            WORKSPACE["width"] = payload.get("width", WORKSPACE["width"])
            WORKSPACE["metatiles"] = payload.get("metatiles", WORKSPACE["metatiles"])

            try:
                with open(WORKSPACE["bin_path"], "wb") as f:
                    for val in WORKSPACE["metatiles"]:
                        f.write(int(val).to_bytes(2, byteorder='little'))
                msg = json.dumps({"message": "Successfully saved to .bin!"}).encode('utf-8')
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(msg)))
                self.end_headers()
                self.wfile.write(msg)
            except Exception as e:
                self.send_response(500)
                self.end_headers()

def main():
    parser = argparse.ArgumentParser(description="Standalone Binary Tilemap Viewer with Tile Drawer")
    parser.add_argument("--bin", required=True, help="Path to tilemap .bin file")
    parser.add_argument("--png", required=True, help="Path to tileset .png file")
    parser.add_argument("--pal", required=True, help="Path to palette .pal file")
    parser.add_argument("--width", type=int, default=20, help="Initial layout grid width")
    parser.add_argument("--port", type=int, default=8080, help="Server port (default: 8080)")

    args = parser.parse_args()

    WORKSPACE["bin_path"] = args.bin
    WORKSPACE["png_path"] = args.png
    WORKSPACE["pal_path"] = args.pal
    WORKSPACE["width"] = args.width

    WORKSPACE["palette"] = load_gba_pal_file(args.pal)

    # Calculate total tiles from PNG dimensions
    if os.path.exists(args.png):
        try:
            img = Image.open(args.png)
            cols = img.width // 8
            rows = img.height // 8
            WORKSPACE["total_tiles_in_png"] = cols * rows
        except Exception:
            WORKSPACE["total_tiles_in_png"] = 512
    else:
        print(f"[ERROR] Could not find tileset PNG: {args.png}")
        sys.exit(1)

    if os.path.exists(args.bin):
        with open(args.bin, "rb") as f:
            while (b := f.read(2)):
                WORKSPACE["metatiles"].append(int.from_bytes(b, byteorder='little'))
    else:
        print(f"[ERROR] Could not find binary map path: {args.bin}")
        sys.exit(1)

    server = ThreadingHTTPServer(('0.0.0.0', args.port), StandaloneMapBackend)
    print(f"[READY] Server running at: http://localhost:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nClean termination.")

if __name__ == "__main__":
    main()

