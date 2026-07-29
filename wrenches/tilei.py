import sys
import os
import csv
import json
import re
import argparse
import io
import shutil
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    from PIL import Image, ImageOps
except ImportError:
    print("[CRITICAL] 'Pillow' library is required. Run: pip install Pillow")
    sys.exit(1)

STUDIO = {
    "folder_path": "",
    "name": "",
    "width": 30,
    "height": 20,
    "tiles_png_path": "",
    "pal_path": "",
    "bin_path": "",
    "palette": [],
    "metatiles": []
}

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

def stage_tilemap_into_studio(folder_path, name, width):
    bin_path = os.path.join(folder_path, f"{name}.bin")
    pal_path = os.path.join(folder_path, f"{name}.pal")
    png_path = os.path.join(folder_path, f"{name}.png")

    if not os.path.exists(png_path):
        print(f"[CRITICAL] PNG file not found: {png_path}")
        sys.exit(1)

    palette = load_gba_pal_file(pal_path) or []

    metatiles = []
    if os.path.exists(bin_path):
        with open(bin_path, "rb") as f:
            while (b := f.read(2)):
                metatiles.append(int.from_bytes(b, byteorder='little'))
    else:
        # Default fallback: Initialize empty 30x20 tilemap if .bin doesn't exist yet
        metatiles = [0] * (width * 20)

    height = max(1, len(metatiles) // width)

    STUDIO["folder_path"] = folder_path
    STUDIO["name"] = name
    STUDIO["width"] = width
    STUDIO["height"] = height
    STUDIO["tiles_png_path"] = png_path
    STUDIO["pal_path"] = pal_path
    STUDIO["bin_path"] = bin_path
    STUDIO["palette"] = palette
    STUDIO["metatiles"] = metatiles

def force_disk_commit(metatiles, width, height):
    STUDIO["width"] = width
    STUDIO["height"] = height
    STUDIO["metatiles"] = metatiles

    try:
        with open(STUDIO["bin_path"], "wb") as f:
            for entry in metatiles:
                f.write(int(entry).to_bytes(2, byteorder='little'))
        print(f"[SUCCESS] Saved tilemap binary to {STUDIO['bin_path']}")
        return True
    except Exception as e:
        print(f"[ERROR] Failed to save binary data: {e}")
        return False

class TilemapWebBackend(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        return

    def compile_tile(self, tile_value):
        png_path = STUDIO["tiles_png_path"]
        if not os.path.exists(png_path):
            return Image.new("RGBA", (8, 8), (40, 40, 40, 255))

        try:
            src_png = Image.open(png_path).convert("P")
            tiles_per_row = src_png.width // 8

            tile_id = tile_value & 0x03FF
            h_flip = (tile_value >> 10) & 0x01
            v_flip = (tile_value >> 11) & 0x01
            palette_num = (tile_value >> 12) & 0x0F

            s_row = tile_id // tiles_per_row
            s_col = tile_id % tiles_per_row
            tile_img_indexed = src_png.crop((s_col * 8, s_row * 8, (s_col + 1) * 8, (s_row + 1) * 8))

            tile_rgba = tile_img_indexed.convert("RGBA")
            pixels = tile_rgba.load()
            active_pal = STUDIO["palette"]

            if active_pal:
                for y_px in range(8):
                    for x_px in range(8):
                        idx_color = tile_img_indexed.getpixel((x_px, y_px))
                        if idx_color % 16 == 0:
                            pixels[x_px, y_px] = (0, 0, 0, 0)
                        else:
                            p_idx = (palette_num * 16) + (idx_color % 16)
                            if p_idx < len(active_pal):
                                r, g, b = active_pal[p_idx]
                                pixels[x_px, y_px] = (r, g, b, 255)
                            else:
                                p_fallback = idx_color % 16
                                if p_fallback < len(active_pal):
                                    r, g, b = active_pal[p_fallback]
                                    pixels[x_px, y_px] = (r, g, b, 255)

            if h_flip:
                tile_rgba = tile_rgba.transpose(Image.FLIP_LEFT_RIGHT)
            if v_flip:
                tile_rgba = tile_rgba.transpose(Image.FLIP_TOP_BOTTOM)

            return tile_rgba
        except Exception:
            return Image.new("RGBA", (8, 8), (0, 0, 0, 255))

    def do_GET(self):
        if self.path.startswith("/?"):
            self.path = "/"

        if self.path == "/":
            # Calculate total tile count from the tilemap PNG
            total_tiles = 512
            if os.path.exists(STUDIO["tiles_png_path"]):
                try:
                    img = Image.open(STUDIO["tiles_png_path"])
                    total_tiles = (img.width // 8) * (img.height // 8)
                except Exception:
                    pass

            html_template = """
            <!DOCTYPE html>
            <html>
            <head>
                <title>GBA Tilemap Studio - __NAME__</title>
                <style>
                    body { background-color: #0b0b0b; color: #00ff66; font-family: 'Courier New', monospace; margin: 0; padding: 15px; display: flex; gap: 15px; height: 98vh; box-sizing: border-box; }
                    .pane { background: #121212; border: 1px solid #00ff66; border-radius: 4px; padding: 12px; display: flex; flex-direction: column; overflow: hidden; }
                    #map-pane { flex: 2; }
                    #sidebar-pane { flex: 1; max-width: 440px; }
                    .grid-container { overflow: auto; background: #000; border: 1px solid #003311; border-radius: 2px; padding: 5px; flex-grow: 1; position: relative; }
                    .matrix { display: grid; gap: 1px; background-color: #051505; width: fit-content; }
                    .tile { width: 24px; height: 24px; background-color: #111; cursor: pointer; user-select: none; border: 1px solid #002208; display: flex; align-items: center; justify-content: center; box-sizing: border-box; position: relative; }
                    .tile img { width: 100%; height: 100%; image-rendering: pixelated; }
                    .tile:hover { border-color: #00ff66; z-index: 2; }
                    .tile.active { border-color: #ffffff !important; box-shadow: 0 0 6px #ffffff; z-index: 3; }
                    .tile.selected-range { background-color: #002244; border-color: #0088ff; opacity: 0.8; }

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
                    .meta-readout { background: #000; padding: 10px; border-radius: 2px; border: 1px solid #002208; font-size: 11px; margin-top: 8px; line-height: 1.5; color: #00ff66; overflow-y: auto; max-height: 250px; }
                    .section-title { font-weight: bold; color: #fff; border-bottom: 1px solid #002208; margin-bottom: 4px; padding-bottom: 2px; }
                    .property-row { display: flex; gap: 4px; margin-top: 6px; align-items: center; }
                    select, input[type="number"], input[type="text"] { background: #000; color: #00ff66; border: 1px solid #00ff66; font-family: monospace; font-size: 11px; padding: 2px; border-radius: 2px; box-sizing: border-box; }
                    .checkbox-label { display: flex; align-items: center; gap: 4px; font-size: 11px; color: #00ff66; cursor: pointer; }
                </style>
            </head>
            <body>
                <div class="pane" id="map-pane">
                    <div class="toolbar">
                        <button onclick="saveStudio()" style="font-weight: bold; border-color: #fff;" title="Ctrl+S">Save Studio</button>
                        <button id="btn-select" onclick="toggleSelectionMode()" title="S">Select Range</button>
                        <button id="btn-hand" class="active-toggle" onclick="setTool('hand')" title="H">Hand (H)</button>
                        <button id="btn-draw" onclick="setTool('draw')" title="D">Draw (D)</button>
                        <button id="btn-picker" onclick="setTool('picker')" title="I">Eyedropper (I)</button>
                        <button onclick="copySelection()" title="C">Copy</button>
                        <button onclick="pasteSelection()" title="V">Paste</button>
                    </div>

                    <div class="grid-container"><div id="map-matrix" class="matrix"></div></div>

                    <div class="toolbar" style="margin-top:8px; margin-bottom:0;">
                        <span style="font-size:11px; align-self:center; color:#fff; margin-right:8px;">SELECTION ATTRIBUTES:</span>
                        <div class="property-row" style="margin-top:0;">
                            <label class="checkbox-label">
                                <input type="checkbox" id="prop-hflip" onchange="applyPropertyField('hFlip', this.checked)"> Flip X
                            </label>
                        </div>
                        <div class="property-row" style="margin-top:0; margin-left:8px;">
                            <label class="checkbox-label">
                                <input type="checkbox" id="prop-vflip" onchange="applyPropertyField('vFlip', this.checked)"> Flip Y
                            </label>
                        </div>
                        <div class="property-row" style="margin-top:0; margin-left:8px;">
                            <label>Palette:</label>
                            <input type="number" id="prop-pal" min="0" max="15" value="0" style="width:40px;" onchange="applyPropertyField('palette', this.value)">
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
                    <div class="section-title">8x8 TILE ATLAS</div>
                    <div class="grid-container" style="display: flex; flex-direction: column;"><div id="atlas-container" class="atlas-visual-matrix"></div></div>
                    <div class="meta-readout" id="readout-box">Select elements to initialize tracking properties.</div>
                </div>

                <script>
                    let STUDIO = __STUDIO_DATA__;
                    let TOTAL_TILES = __TOTAL_TILES__;

                    let state = {
                        selectionActive: false, selectionStart: null,
                        cursorIdx: 0, clipboard: null, currentTool: 'hand',
                        lastActiveTool: 'hand', selectedTileId: 0
                    };

                    function parseTileValue(val) {
                        return {
                            id: val & 0x03FF,
                            hFlip: ((val >> 10) & 0x01) === 1,
                            vFlip: ((val >> 11) & 0x01) === 1,
                            palette: (val >> 12) & 0x0F
                        };
                    }

                    function packTileValue(id, hFlip, vFlip, palette) {
                        return (id & 0x03FF) | ((hFlip ? 1 : 0) << 10) | ((vFlip ? 1 : 0) << 11) | ((palette & 0x0F) << 12);
                    }

                    function setTool(toolName) {
                        if (state.currentTool !== 'picker') state.lastActiveTool = state.currentTool;
                        state.currentTool = toolName;
                        document.getElementById("btn-hand").classList.toggle("active-toggle", toolName === 'hand');
                        document.getElementById("btn-draw").classList.toggle("active-toggle", toolName === 'draw');
                        document.getElementById("btn-picker").classList.toggle("active-toggle", toolName === 'picker');
                    }

                    function getSelectedIndices() {
                        let width = STUDIO.width;
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
                        let width = STUDIO.width;
                        let tiles = STUDIO.metatiles;
                        grid.style.gridTemplateColumns = `repeat(${width}, 24px)`;

                        tiles.forEach((entry, idx) => {
                            let cell = document.createElement("div"); cell.className = "tile"; cell.id = `tile-${idx}`;
                            let img = document.createElement("img");
                            img.src = `/render_tile?val=${entry}`;
                            cell.appendChild(img);

                            let cX = idx % width;
                            let cY = Math.floor(idx / width);

                            if (idx === state.cursorIdx) cell.classList.add("active");
                            if (state.selectionActive && state.selectionStart !== null) {
                                let sX = state.selectionStart % width, sY = Math.floor(state.selectionStart / width);
                                let curX = state.cursorIdx % width, curY = Math.floor(state.cursorIdx / width);
                                if (cX >= Math.min(sX, curX) && cX <= Math.max(sX, curX) && cY >= Math.min(sY, curY) && cY <= Math.max(sY, curY)) cell.classList.add("selected-range");
                            }

                            cell.onclick = () => {
                                state.cursorIdx = idx;
                                let parsed = parseTileValue(tiles[idx]);
                                if (state.currentTool === 'picker') {
                                    state.selectedTileId = parsed.id;
                                    setTool(state.lastActiveTool);
                                    buildVisualAtlas();
                                } else if (state.currentTool === 'draw') {
                                    applyTileToSelection(state.selectedTileId);
                                }
                                renderMatrixGrid(); updateReadout();
                            };

                            grid.appendChild(cell);
                        });
                    }

                    function buildVisualAtlas() {
                        const container = document.getElementById("atlas-container"); container.innerHTML = "";

                        for (let tileId = 0; tileId < TOTAL_TILES; tileId++) {
                            let cell = document.createElement("div"); cell.className = "atlas-cell"; cell.id = `atlas-cell-${tileId}`;
                            let img = document.createElement("img");

                            let dummyVal = packTileValue(tileId, false, false, 0);
                            img.src = `/render_tile?val=${dummyVal}`;
                            cell.appendChild(img);

                            let tag = document.createElement("div"); tag.className = "cell-id-tag"; tag.innerText = tileId;
                            cell.appendChild(tag);

                            if (tileId === state.selectedTileId) cell.classList.add("tracker-highlight");

                            cell.onclick = () => {
                                state.selectedTileId = tileId;
                                Array.from(container.children).forEach(c => c.classList.remove("tracker-highlight"));
                                cell.classList.add("tracker-highlight");

                                if (state.currentTool === 'draw') {
                                    applyTileToSelection(tileId);
                                }
                            };
                            container.appendChild(cell);
                        }
                    }

                    function applyTileToSelection(tileId) {
                        let tiles = STUDIO.metatiles;
                        let targets = getSelectedIndices();

                        targets.forEach(idx => {
                            if (idx < tiles.length) {
                                let old = parseTileValue(tiles[idx]);
                                tiles[idx] = packTileValue(tileId, old.hFlip, old.vFlip, old.palette);
                            }
                        });
                        renderMatrixGrid(); updateReadout();
                    }

                    function applyPropertyField(field, val) {
                        let tiles = STUDIO.metatiles;
                        let targets = getSelectedIndices();

                        targets.forEach(idx => {
                            if (idx < tiles.length) {
                                let t = parseTileValue(tiles[idx]);
                                if (field === 'hFlip') t.hFlip = Boolean(val);
                                else if (field === 'vFlip') t.vFlip = Boolean(val);
                                else if (field === 'palette') t.palette = parseInt(val) || 0;

                                tiles[idx] = packTileValue(t.id, t.hFlip, t.vFlip, t.palette);
                            }
                        });
                        renderMatrixGrid(); updateReadout();
                    }

                    function resizeMapDimensions(dim, val) {
                        let newV = parseInt(val) || 1;
                        let oldW = STUDIO.width, oldH = STUDIO.height;
                        let newW = (dim === 'width') ? newV : oldW;
                        let newH = (dim === 'height') ? newV : oldH;

                        let newTiles = new Array(newW * newH).fill(0);
                        for (let y = 0; y < Math.min(oldH, newH); y++) {
                            for (let x = 0; x < Math.min(oldW, newW); x++) {
                                newTiles[y * newW + x] = STUDIO.metatiles[y * oldW + x];
                            }
                        }
                        STUDIO.width = newW; STUDIO.height = newH; STUDIO.metatiles = newTiles;
                        renderMatrixGrid();
                    }

                    function updateReadout() {
                        let width = STUDIO.width;
                        let tiles = STUDIO.metatiles;
                        let t = parseTileValue(tiles[state.cursorIdx] || 0);

                        document.getElementById("prop-hflip").checked = t.hFlip;
                        document.getElementById("prop-vflip").checked = t.vFlip;
                        document.getElementById("prop-pal").value = t.palette;
                        document.getElementById("prop-width").value = STUDIO.width;
                        document.getElementById("prop-height").value = STUDIO.height;

                        let cX = state.cursorIdx % width;
                        let cY = Math.floor(state.cursorIdx / width);

                        document.getElementById("readout-box").innerHTML = `
                            <div class="section-title">TILE DETAILS</div>
                            <strong>Position:</strong> X: ${cX}, Y: ${cY}<br>
                            <strong>8x8 Tile ID:</strong> ${t.id}<br>
                            <strong>Horizontal Flip:</strong> ${t.hFlip ? "YES" : "NO"}<br>
                            <strong>Vertical Flip:</strong> ${t.vFlip ? "YES" : "NO"}<br>
                            <strong>Palette Index:</strong> ${t.palette}<br>
                            <strong>Active Tool:</strong> ${state.currentTool.toUpperCase()}<br>
                            <strong>Selected Brush ID:</strong> ${state.selectedTileId}
                        `;
                    }

                    function toggleSelectionMode() {
                        state.selectionActive = !state.selectionActive;
                        state.selectionStart = state.selectionActive ? state.cursorIdx : null;
                        document.getElementById("btn-select").classList.toggle("active-toggle", state.selectionActive);
                        renderMatrixGrid();
                    }

                    function copySelection() {
                        if (!state.selectionActive || state.selectionStart === null) return;
                        let width = STUDIO.width;
                        let sX = state.selectionStart % width, sY = Math.floor(state.selectionStart / width);
                        let curX = state.cursorIdx % width, curY = Math.floor(state.cursorIdx / width);
                        let x1 = Math.min(sX, curX), x2 = Math.max(sX, curX), y1 = Math.min(sY, curY), y2 = Math.max(sY, curY);

                        state.clipboard = { w: x2 - x1 + 1, h: y2 - y1 + 1, blocks: [] };
                        for(let y=y1; y<=y2; y++) {
                            for(let x=x1; x<=x2; x++) {
                                state.clipboard.blocks.push(STUDIO.metatiles[y * width + x]);
                            }
                        }
                        state.selectionActive = false;
                        document.getElementById("btn-select").classList.remove("active-toggle");
                        renderMatrixGrid();
                    }

                    function pasteSelection() {
                        if (!state.clipboard) return;
                        let width = STUDIO.width;
                        let height = STUDIO.height;
                        let startX = state.cursorIdx % width, startY = Math.floor(state.cursorIdx / width);

                        for(let y=0; y<state.clipboard.h; y++) {
                            if (startY + y >= height) break;
                            for(let x=0; x<state.clipboard.w; x++) {
                                if (startX + x >= width) break;
                                STUDIO.metatiles[(startY + y) * width + (startX + x)] = state.clipboard.blocks[y * state.clipboard.w + x];
                            }
                        }
                        renderMatrixGrid(); updateReadout();
                    }

                    function saveStudio() {
                        fetch('/save', {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify({
                                width: STUDIO.width,
                                height: STUDIO.height,
                                metatiles: STUDIO.metatiles
                            })
                        })
                        .then(res => res.json()).then(data => alert(data.message));
                    }

                    window.addEventListener("keydown", (e) => {
                        if (document.activeElement.tagName === "INPUT") return;
                        let width = STUDIO.width;
                        let maxLen = STUDIO.metatiles.length;

                        if (e.key === "ArrowUp" && state.cursorIdx >= width) state.cursorIdx -= width;
                        else if (e.key === "ArrowDown" && state.cursorIdx + width < maxLen) state.cursorIdx += width;
                        else if (e.key === "ArrowLeft" && state.cursorIdx % width > 0) state.cursorIdx -= 1;
                        else if (e.key === "ArrowRight" && state.cursorIdx % width < width - 1) state.cursorIdx += 1;
                        else if (e.key.toLowerCase() === "s" && e.ctrlKey) { e.preventDefault(); saveStudio(); return; }
                        else if (e.key.toLowerCase() === "s") { toggleSelectionMode(); return; }
                        else if (e.key.toLowerCase() === "c") { copySelection(); return; }
                        else if (e.key.toLowerCase() === "v") { pasteSelection(); return; }
                        else if (e.key.toLowerCase() === "h") { setTool('hand'); return; }
                        else if (e.key.toLowerCase() === "d") { setTool('draw'); return; }
                        else if (e.key.toLowerCase() === "i") { setTool('picker'); return; }
                        else return;
                        renderMatrixGrid(); updateReadout();
                    });

                    renderMatrixGrid();
                    buildVisualAtlas();
                    updateReadout();
                </script>
            </body>
            </html>
            """.replace("__STUDIO_DATA__", json.dumps(STUDIO)).replace("__TOTAL_TILES__", str(total_tiles)).replace("__NAME__", STUDIO["name"])

            encoded_html = html_template.encode('utf-8')

            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(encoded_html)))
            self.end_headers()
            self.wfile.write(encoded_html)

        elif self.path.startswith("/render_tile"):
            val_params = re.findall(r'val=(\d+)', self.path)
            tile_val = int(val_params[0]) if val_params else 0

            tile_img = self.compile_tile(tile_val)
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

    def do_POST(self):
        if self.path == "/save":
            content_length = int(self.headers['Content-Length'])
            data = json.loads(self.rfile.read(content_length).decode('utf-8'))

            width = data.get("width", STUDIO["width"])
            height = data.get("height", STUDIO["height"])
            metatiles = data.get("metatiles", STUDIO["metatiles"])

            if force_disk_commit(metatiles, width, height):
                res_msg = json.dumps({"message": f"Successfully updated binary tilemap: '{STUDIO['bin_path']}'"}).encode('utf-8')
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(res_msg)))
                self.end_headers()
                self.wfile.write(res_msg)
                return
            self.send_response(500)
            self.end_headers()

def main():
    parser = argparse.ArgumentParser(description="GBA 8x8 Tilemap Binary Editor Studio")
    parser.add_argument("folder_path", help="Path to folder containing the target tileset files (e.g., graphics/title_screen)")
    parser.add_argument("tilemap_name", help="Base file name of target files (e.g., icefallstiles)")
    parser.add_argument("--width", "-w", type=int, default=30, help="Tilemap column width grid count (defaults to 30)")
    args = parser.parse_args()

    stage_tilemap_into_studio(args.folder_path, args.tilemap_name, args.width)

    server = ThreadingHTTPServer(('0.0.0.0', 8080), TilemapWebBackend)
    print(f"GBA 8x8 Tilemap Studio live at: http://localhost:8080")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nClean termination.")

if __name__ == "__main__":
    main()

