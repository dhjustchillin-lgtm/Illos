import argparse
import os
import re
import struct

# Layer types definition
LAYER_TYPES = {
    0: "Normal (Middle & Top)",
    1: "Covered (Bottom & Middle)",
    2: "Split (Bottom & Top)",
}

# Terrain Types (32-bit format)
TERRAIN_TYPES = {
    0: "Normal",
    1: "Grass",
    2: "Water",
    3: "Waterfall",
    4: "Sand",
    5: "Ice",
}

# Encounter Types (32-bit format)
ENCOUNTER_TYPES = {
    0: "None",
    1: "Land / Grass",
    2: "Water",
}


def load_behavior_constants(header_path: str) -> dict:
    """Parses include/constants/metatile_behaviors.h for both enums and #defines."""
    behaviors = {}
    if not os.path.exists(header_path):
        print(f"Warning: Header file '{header_path}' not found. Behavior names won't resolve.")
        return behaviors

    with open(header_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # 1. Parse Enums: enum { MB_NORMAL, MB_SECRET_BASE_WALL, ... };
    enum_matches = re.findall(r"enum\s*\{([^}]+)\}", content, re.MULTILINE | re.DOTALL)
    for enum_body in enum_matches:
        current_val = 0
        # Clean out comments
        enum_body = re.sub(r"//.*|/\*[\s\S]*?\*/", "", enum_body)

        for item in enum_body.split(","):
            item = item.strip()
            if not item:
                continue

            if "=" in item:
                name, val_str = item.split("=", 1)
                name = name.strip()
                val_str = val_str.strip()
                current_val = int(val_str, 16) if val_str.startswith("0x") else int(val_str)
            else:
                name = item.strip()

            behaviors[current_val] = name
            current_val += 1

    # 2. Parse #defines: #define MB_INVALID 255
    define_pattern = re.compile(r"#define\s+(MB_\w+)\s+(0x[0-9A-Fa-f]+|\d+|UCHAR_MAX)")
    for match in define_pattern.finditer(content):
        name, val_str = match.groups()
        if val_str == "UCHAR_MAX":
            val = 255
        else:
            val = int(val_str, 16) if val_str.startswith("0x") else int(val_str)
        behaviors[val] = name

    return behaviors


def parse_attributes(file_path: str, metatile_id: int, entry_bytes: int, behaviors: dict):
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        return

    file_size = os.path.getsize(file_path)
    total_metatiles = file_size // entry_bytes

    if metatile_id >= total_metatiles:
        print(f"Error: Metatile ID {metatile_id} (0x{metatile_id:X}) is out of range.")
        print(f"File size is {file_size} bytes ({total_metatiles} metatiles at {entry_bytes} bytes/entry).")
        return

    offset = metatile_id * entry_bytes

    with open(file_path, "rb") as f:
        f.seek(offset)
        raw_bytes = f.read(entry_bytes)

    print(f"=== Metatile ID: {metatile_id} (0x{metatile_id:03X}) [{entry_bytes * 8}-bit Mode] ===")

    if entry_bytes == 2:
        val = struct.unpack("<H", raw_bytes)[0]
        behavior = val & 0x00FF
        layer_type = (val & 0xF000) >> 12

        behavior_name = behaviors.get(behavior, "Custom / Unmapped")

        print(f"Raw Hex       : 0x{val:04X}")
        print(f"Layer Type    : {layer_type} ({LAYER_TYPES.get(layer_type, 'Unknown')})")
        print(f"Behavior      : {behavior} (0x{behavior:02X}) -> {behavior_name}")

    elif entry_bytes == 4:
        val = struct.unpack("<I", raw_bytes)[0]
        behavior = val & 0x000001FF
        terrain_type = (val & 0x00003E00) >> 9
        encounter_type = (val & 0x07000000) >> 24
        layer_type = (val & 0x60000000) >> 29

        behavior_name = behaviors.get(behavior, "Custom / Unmapped")

        print(f"Raw Hex       : 0x{val:08X}")
        print(f"Layer Type    : {layer_type} ({LAYER_TYPES.get(layer_type, 'Unknown')})")
        print(f"Behavior      : {behavior} (0x{behavior:03X}) -> {behavior_name}")
        print(f"Terrain Type  : {terrain_type} ({TERRAIN_TYPES.get(terrain_type, 'Unknown')})")
        print(f"Encounter Type: {encounter_type} ({ENCOUNTER_TYPES.get(encounter_type, 'Unknown')})")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Parse GBA Pokemon Metatile Attributes")
    parser.add_argument("file", help="Path to metatile_attributes.bin")
    parser.add_argument("id", help="Metatile ID (decimal or hex like 0x21)")
    parser.add_argument(
        "-b",
        "--bytes",
        type=int,
        choices=[2, 4],
        default=2,
        help="Bytes per entry (2 for standard Emerald, 4 for expanded/FireRed). Default: 2",
    )
    parser.add_argument(
        "--header",
        default="../include/constants/metatile_behaviors.h",
        help="Path to metatile_behaviors.h",
    )

    args = parser.parse_args()
    tile_id = int(args.id, 0)

    behavior_map = load_behavior_constants(args.header)
    parse_attributes(args.file, tile_id, args.bytes, behavior_map)
