#!/usr/bin/env python3

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

from PIL import Image


TILE_SIZE = 8
TILE_BYTES_4BPP = 32
MAX_TEXT_BG_TILE = 1023


def read_palette(path: Path):
    raw = path.read_bytes()

    if len(raw) % 2:
        raise ValueError(f"{path}: palette size must be even")

    colors = []

    for offset in range(0, len(raw), 2):
        value = struct.unpack_from("<H", raw, offset)[0]

        # GBA BGR555 format.
        r = ((value >> 0) & 0x1F) << 3
        g = ((value >> 5) & 0x1F) << 3
        b = ((value >> 10) & 0x1F) << 3

        colors.append((r, g, b))

    return colors


def make_palette(colors):
    palette = []

    for color in colors:
        palette.extend(color)

    # Pillow requires a 256-color palette for indexed PNGs.
    palette.extend([0, 0, 0] * (256 - len(colors)))
    return palette


def decode_4bpp_tile(raw):
    if len(raw) != TILE_BYTES_4BPP:
        raise ValueError("A 4bpp tile must contain exactly 32 bytes")

    pixels = []

    for value in raw:
        # GBA 4bpp stores the first pixel in the low nibble.
        pixels.append(value & 0x0F)
        pixels.append((value >> 4) & 0x0F)

    return pixels


def read_tiles(path: Path):
    raw = path.read_bytes()

    if len(raw) % TILE_BYTES_4BPP:
        raise ValueError(
            f"{path}: size {len(raw)} is not divisible by 32; "
            "this does not look like uncompressed 4bpp tile data"
        )

    tiles = []

    for offset in range(0, len(raw), TILE_BYTES_4BPP):
        tiles.append(decode_4bpp_tile(raw[offset:offset + TILE_BYTES_4BPP]))

    return tiles


def render_tileset(tiles, palette, columns):
    if not tiles:
        raise ValueError("The graphics file contains no tiles")

    rows = (len(tiles) + columns - 1) // columns
    image = Image.new("P", (columns * 8, rows * 8), 0)
    image.putpalette(make_palette(palette))

    for tile_index, tile in enumerate(tiles):
        tile_x = tile_index % columns
        tile_y = tile_index // columns

        for y in range(8):
            for x in range(8):
                image.putpixel(
                    (tile_x * 8 + x, tile_y * 8 + y),
                    tile[y * 8 + x],
                )

    return image


def flip_tile(tile, h_flip, v_flip):
    output = [0] * 64

    for y in range(8):
        for x in range(8):
            source_x = 7 - x if h_flip else x
            source_y = 7 - y if v_flip else y
            output[y * 8 + x] = tile[source_y * 8 + source_x]

    return output


def read_map(path: Path):
    raw = path.read_bytes()

    if len(raw) % 2:
        raise ValueError(f"{path}: map size must be even")

    return list(struct.unpack("<" + "H" * (len(raw) // 2), raw))


def decode_map_entry(entry):
    tile_index = entry & 0x03FF
    h_flip = bool(entry & 0x0400)
    v_flip = bool(entry & 0x0800)
    palette_bank = (entry >> 12) & 0x0F

    return tile_index, h_flip, v_flip, palette_bank


def analyse_map(entries, map_name, tile_count):
    invalid_entries = []
    used_tiles = set()
    palette_banks = set()
    h_flipped = 0
    v_flipped = 0

    for position, entry in enumerate(entries):
        tile_index, h_flip, v_flip, palette_bank = decode_map_entry(entry)

        used_tiles.add(tile_index)
        palette_banks.add(palette_bank)

        if h_flip:
            h_flipped += 1

        if v_flip:
            v_flipped += 1

        # This detects the actual failure mode: a map tile index
        # outside the available graphics.
        if tile_index >= tile_count:
            invalid_entries.append(
                (position, entry, tile_index, h_flip, v_flip, palette_bank)
            )

    print(f"\nMap: {map_name}")
    print(f"  Entries:       {len(entries)}")
    print(f"  Used tiles:    {len(used_tiles)}")
    print(f"  Lowest tile:   {min(used_tiles) if used_tiles else 'none'}")
    print(f"  Highest tile:  {max(used_tiles) if used_tiles else 'none'}")
    print(f"  Palette banks: {sorted(palette_banks)}")
    print(f"  H-flipped:     {h_flipped}")
    print(f"  V-flipped:     {v_flipped}")
    print(f"  Invalid refs:  {len(invalid_entries)}")

    if max(used_tiles, default=0) > MAX_TEXT_BG_TILE:
        print(
            "  WARNING: map uses tile indices above 1023. "
            "Those cannot be represented by a normal GBA text-BG map."
        )

    if invalid_entries:
        print("  First invalid references:")

        for item in invalid_entries[:10]:
            position, entry, tile_index, h_flip, v_flip, palette_bank = item
            print(
                f"    entry {position}: "
                f"raw=0x{entry:04X}, "
                f"tile={tile_index}, "
                f"hflip={int(h_flip)}, "
                f"vflip={int(v_flip)}, "
                f"palbank={palette_bank}"
            )

    return invalid_entries


def render_map(entries, tiles, palette, width, height, stride):
    expected = stride * height

    if len(entries) < expected:
        raise ValueError(
            f"Map has {len(entries)} entries, but "
            f"{width}x{height} with stride {stride} requires {expected}"
        )

    image = Image.new("P", (width * 8, height * 8), 0)
    image.putpalette(make_palette(palette))

    for y in range(height):
        for x in range(width):
            entry = entries[y * stride + x]
            tile_index, h_flip, v_flip, palette_bank = decode_map_entry(entry)

            if tile_index >= len(tiles):
                # Render invalid references in bright magenta so they
                # are immediately visible in the output image.
                for py in range(8):
                    for px in range(8):
                        image.putpixel(
                            (x * 8 + px, y * 8 + py),
                            15,
                        )
                continue

            tile = tiles[tile_index]

            if h_flip or v_flip:
                tile = flip_tile(tile, h_flip, v_flip)

            # A normal 4bpp text map can select a palette bank.
            # For a 16-color palette, bank 0 is the only valid bank.
            if palette_bank != 0:
                # The source palette supplied by -pS is normally only
                # one 16-color bank. Mark nonzero-bank entries visibly.
                for py in range(8):
                    for px in range(8):
                        image.putpixel(
                            (x * 8 + px, y * 8 + py),
                            14,
                        )
                continue

            for py in range(8):
                for px in range(8):
                    image.putpixel(
                        (x * 8 + px, y * 8 + py),
                        tile[py * 8 + px],
                    )

    return image


def parse_args():
    parser = argparse.ArgumentParser(
        description="Inspect and render GBA 4bpp graphics and text BG maps."
    )

    parser.add_argument(
        "--gfx",
        required=True,
        type=Path,
        help="GBA 4bpp graphics binary, such as test_bg.img.bin",
    )

    parser.add_argument(
        "--pal",
        required=True,
        type=Path,
        help="GBA BGR555 palette binary, such as test_bg.pal.bin",
    )

    parser.add_argument(
        "--map",
        dest="maps",
        action="append",
        type=Path,
        help="Map binary. Repeat this option for multiple maps.",
    )

    parser.add_argument(
        "--tileset-out",
        type=Path,
        default=Path("tileset.png"),
        help="Output filename for the linear tileset preview.",
    )

    parser.add_argument(
        "--map-out",
        type=Path,
        default=Path("map.png"),
        help="Output filename for the rendered map.",
    )

    parser.add_argument(
        "--map-width",
        type=int,
        default=30,
        help="Visible map width in tiles. Default: 30.",
    )

    parser.add_argument(
        "--map-height",
        type=int,
        default=20,
        help="Visible map height in tiles. Default: 20.",
    )

    parser.add_argument(
        "--map-stride",
        type=int,
        default=30,
        help="Entries between map rows. Default: 30.",
    )

    parser.add_argument(
        "--tileset-columns",
        type=int,
        default=16,
        help="Tileset preview width in tiles. Default: 16.",
    )

    parser.add_argument(
        "--no-tileset",
        action="store_true",
        help="Do not write the linear tileset preview.",
    )

    parser.add_argument(
        "--no-render",
        action="store_true",
        help="Only analyse the maps; do not render a map preview.",
    )

    return parser.parse_args()


def main():
    args = parse_args()

    try:
        palette = read_palette(args.pal)
        tiles = read_tiles(args.gfx)

        print(f"Graphics: {args.gfx}")
        print(f"  Bytes:  {args.gfx.stat().st_size}")
        print(f"  Tiles:  {len(tiles)}")

        print(f"\nPalette: {args.pal}")
        print(f"  Bytes:  {args.pal.stat().st_size}")
        print(f"  Colors: {len(palette)}")

        if len(palette) != 16:
            print(
                "  WARNING: a 4bpp palette bank should contain 16 colors "
                "(32 bytes)."
            )

        if len(tiles) > 1024:
            print(
                "\nWARNING: the graphics contain more than 1024 tiles. "
                "A regular GBA text-BG map cannot reference all of them."
            )

        if not args.no_tileset:
            tileset = render_tileset(
                tiles,
                palette,
                args.tileset_columns,
            )
            tileset.save(args.tileset_out)
            print(f"\nSaved tileset preview: {args.tileset_out}")

        if not args.maps:
            return 0

        for map_path in args.maps:
            entries = read_map(map_path)
            analyse_map(entries, map_path.name, len(tiles))

        if not args.no_render:
            first_map = args.maps[0]
            entries = read_map(first_map)

            rendered = render_map(
                entries,
                tiles,
                palette,
                args.map_width,
                args.map_height,
                args.map_stride,
            )

            rendered.save(args.map_out)
            print(f"\nSaved map preview: {args.map_out}")

    except FileNotFoundError as error:
        print(f"error: file not found: {error.filename}", file=sys.stderr)
        return 1
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())