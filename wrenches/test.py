import struct
import sys


def parse_metatile_attributes(bin_path: str, tile_id: int):
    # Each metatile attribute entry is 2 bytes (16-bit unsigned short)
    entry_size = 2

    try:
        with open(bin_path, "rb") as f:
            f.seek(tile_id * entry_size)
            data = f.read(entry_size)

            if len(data) < entry_size:
                print(f"Error: Tile ID {tile_id} is out of bounds for file.")
                return

            # Unpack 16-bit unsigned integer (little-endian)
            (attr,) = struct.unpack("<H", data)

            # Bitwise extraction
            behavior = attr & 0x01FF
            terrain = (attr >> 9) & 0x07
            encounter = (attr >> 12) & 0x03
            layer = (attr >> 14) & 0x03

            print(f"--- Properties for Tile ID {tile_id} (0x{tile_id:X}) ---")
            print(f"Raw Value        : 0x{attr:04X}")
            print(f"Metatile Behavior: {behavior} (0x{behavior:03X})")
            print(f"Terrain Type     : {terrain}")
            print(f"Encounter Type   : {encounter}")
            print(f"Layer Type       : {layer}")

    except FileNotFoundError:
        print(f"Error: File not found at '{bin_path}'")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python script.py <path_to_metatile_attributes.bin> <tile_id>")
        sys.exit(1)

    filepath = sys.argv[1]
    metatile_id = int(sys.argv[2], 0)  # Supports both decimal (5) and hex (0x5)

    parse_metatile_attributes(filepath, metatile_id)
