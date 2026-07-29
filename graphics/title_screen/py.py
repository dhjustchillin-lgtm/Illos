from PIL import Image
import struct

IMG_BIN = "icefallstiles.img.bin"
PAL_BIN = "icefallstiles.pal.bin"
PNG_OUT = "icefallstiles.png"
PAL_OUT = "icefallstiles.pal"

# 1. Read Palette (16 colors, 16-bit BGR555 format)
colors = []
with open(PAL_BIN, "rb") as f:
    pal_bytes = f.read()
    for i in range(0, min(len(pal_bytes), 32), 2):
        bgr555 = struct.unpack("<H", pal_bytes[i:i+2])[0]
        r = (bgr555 & 0x1F) << 3
        g = ((bgr555 >> 5) & 0x1F) << 3
        b = ((bgr555 >> 10) & 0x1F) << 3
        colors.append((r, g, b))

# Pad palette to 256 colors for standard PNG indexed palette
flattened_pal = []
for c in colors:
    flattened_pal.extend(c)
flattened_pal.extend([0, 0, 0] * (256 - len(colors)))

# 2. Export JASC-PAL file (for GBA graphics pipeline)
with open(PAL_OUT, "w") as f:
    f.write("JASC-PAL\n0100\n16\n")
    for r, g, b in colors[:16]:
        f.write(f"{r} {g} {b}\n")
print(f"Saved palette: {PAL_OUT}")

# 3. Read 4bpp Tile Data & Reconstruct Tileset Image
with open(IMG_BIN, "rb") as f:
    img_bytes = f.read()

# Each 8x8 4bpp tile is 32 bytes (2 pixels per byte)
num_tiles = len(img_bytes) // 32
cols = 16  # Standard 128px width grid (16 tiles wide)
rows = (num_tiles + cols - 1) // cols

indexed_pixels = []
for tile_idx in range(num_tiles):
    tile_data = img_bytes[tile_idx * 32 : (tile_idx + 1) * 32]
    # Unpack 4bpp nibbles into pixel indices
    tile_pixels = []
    for byte in tile_data:
        p1 = byte & 0x0F
        p2 = (byte >> 4) & 0x0F
        tile_pixels.extend([p1, p2])
    indexed_pixels.append(tile_pixels)

# Arrange tiles into grid
img_data = bytearray(cols * 8 * rows * 8)
for tile_idx, tile_pix in enumerate(indexed_pixels):
    tx = tile_idx % cols
    ty = tile_idx // cols
    for row in range(8):
        for col in range(8):
            px = tx * 8 + col
            py = ty * 8 + row
            img_data[py * (cols * 8) + px] = tile_pix[row * 8 + col]

# Save indexed PNG
img = Image.frombytes("P", (cols * 8, rows * 8), bytes(img_data))
img.putpalette(flattened_pal)
img.save(PNG_OUT)

print(f"Saved PNG tileset: {PNG_OUT}")

