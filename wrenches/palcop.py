import sys
import struct

def copy_tilemap_palettes(source_path, target_path):
    # Read both binary files
    with open(source_path, "rb") as f_src, open(target_path, "rb") as f_tgt:
        src_data = f_src.read()
        tgt_data = f_tgt.read()

    # GBA tilemap entries are 16-bit (2 bytes per entry)
    if len(src_data) != len(tgt_data):
        print("Warning: Tilemap file sizes do not match!")
    
    num_entries = min(len(src_data), len(tgt_data)) // 2

    # Unpack entries as little-endian 16-bit unsigned integers ('<H')
    src_entries = struct.unpack(f"<{num_entries}H", src_data[:num_entries * 2])
    tgt_entries = struct.unpack(f"<{num_entries}H", tgt_data[:num_entries * 2])

    modified_entries = []
    for src_val, tgt_val in zip(src_entries, tgt_entries):
        # Bit breakdown of a GBA 16-bit tilemap entry:
        # - Bits 0–9  (0x03FF): Tile ID
        # - Bit 10    (0x0400): Horizontal Flip
        # - Bit 11    (0x0800): Vertical Flip
        # - Bits 12–15(0xF000): Palette Index
        
        tgt_base = tgt_val & 0x0FFF  # Keep Tile ID + Flips from target
        src_pal  = src_val & 0xF000  # Extract Palette Index from source
        
        combined = tgt_base | src_pal
        modified_entries.append(combined)

    # Repack and overwrite the target file
    output_data = struct.pack(f"<{len(modified_entries)}H", *modified_entries)
    with open(target_path, "wb") as f_out:
        f_out.write(output_data)

    print(f"Successfully copied palette data from '{source_path}' into '{target_path}'.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python copy_palettes.py <source_tilemap.bin> <target_tilemap.bin>")
        sys.exit(1)

    copy_tilemap_palettes(sys.argv[1], sys.argv[2])

