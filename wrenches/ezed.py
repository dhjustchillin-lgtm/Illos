#!/usr/bin/env python3

import argparse
import curses
import os
import sys
from pathlib import Path

from PIL import Image


BLOCK = "██"
SELECTED_BLOCK = "▓▓"
PALETTE_SIZE = 16


def clamp(value, low=0, high=255):
    return max(low, min(high, value))


def read_jasc_palette(path):
    lines = [
        line.strip()
        for line in Path(path).read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]

    if len(lines) < 3 or lines[0].upper() != "JASC-PAL":
        raise ValueError(f"{path} is not a valid JASC-PAL file")

    try:
        color_count = int(lines[2])
    except ValueError as exc:
        raise ValueError("Invalid JASC palette color count") from exc

    colors = []

    for line in lines[3:3 + color_count]:
        parts = line.split()

        if len(parts) != 3:
            raise ValueError(f"Invalid palette line: {line}")

        try:
            rgb = tuple(clamp(int(value)) for value in parts)
        except ValueError as exc:
            raise ValueError(f"Invalid palette line: {line}") from exc

        colors.append(rgb)

    while len(colors) < PALETTE_SIZE:
        colors.append((0, 0, 0))

    return colors[:PALETTE_SIZE]


def write_jasc_palette(path, palette):
    lines = [
        "JASC-PAL",
        "0100",
        str(PALETTE_SIZE),
    ]

    for r, g, b in palette[:PALETTE_SIZE]:
        lines.append(f"{r} {g} {b}")

    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


def load_4bpp_image(path):
    image = Image.open(path).convert("P")
    pixels = [value & 0x0F for value in image.getdata()]
    return image.width, image.height, pixels


def save_4bpp_image(path, width, height, pixels, palette):
    image = Image.new("P", (width, height))
    image.putdata([value & 0x0F for value in pixels])

    flat_palette = []

    for r, g, b in palette[:PALETTE_SIZE]:
        flat_palette.extend((r, g, b))

    flat_palette.extend([0] * (768 - len(flat_palette)))
    image.putpalette(flat_palette)
    image.save(path)


def nearest_xterm_color(rgb):
    base_colors = [
        (0, 0, 0),
        (128, 0, 0),
        (0, 128, 0),
        (128, 128, 0),
        (0, 0, 128),
        (128, 0, 128),
        (0, 128, 128),
        (192, 192, 192),
        (128, 128, 128),
        (255, 0, 0),
        (0, 255, 0),
        (255, 255, 0),
        (0, 0, 255),
        (255, 0, 255),
        (0, 255, 255),
        (255, 255, 255),
    ]

    colors = list(base_colors)

    levels = (0, 95, 135, 175, 215, 255)

    for red in levels:
        for green in levels:
            for blue in levels:
                colors.append((red, green, blue))

    for value in range(8, 239, 10):
        colors.append((value, value, value))

    def distance(color):
        return sum(
            (component - target) ** 2
            for component, target in zip(rgb, color)
        )

    return min(range(len(colors)), key=lambda index: distance(colors[index]))


def key_to_index(key):
    if ord("0") <= key <= ord("9"):
        return key - ord("0")

    if ord("a") <= key <= ord("f"):
        return key - ord("a") + 10

    if ord("A") <= key <= ord("F"):
        return key - ord("A") + 10

    return None


class Editor:
    def __init__(self, image_path, palette_path):
        self.image_path = image_path
        self.palette_path = palette_path

        self.width, self.height, self.pixels = load_4bpp_image(image_path)
        self.palette = read_jasc_palette(palette_path)

        self.cursor_x = 0
        self.cursor_y = 0
        self.selected_index = 0

        self.view_x = 0
        self.view_y = 0

        self.modified = False
        self.palette_modified = False

    def pixel(self, x, y):
        return self.pixels[y * self.width + x]

    def set_pixel(self, x, y, value):
        self.pixels[y * self.width + x] = value & 0x0F
        self.modified = True

    def move(self, dx, dy):
        self.cursor_x = max(
            0,
            min(self.width - 1, self.cursor_x + dx),
        )

        self.cursor_y = max(
            0,
            min(self.height - 1, self.cursor_y + dy),
        )

    def refresh_color_pairs(self):
        for index, rgb in enumerate(self.palette):
            terminal_color = nearest_xterm_color(rgb)

            try:
                curses.init_pair(
                    index + 1,
                    terminal_color,
                    -1,  # terminal default background
                )
            except curses.error:
                curses.init_pair(
                    index + 1,
                    curses.COLOR_WHITE,
                    -1,
                )

    def ensure_visible(self, rows, cols):
        visible_width = max(1, cols // 2)
        visible_height = max(1, rows - 3)

        if self.cursor_x < self.view_x:
            self.view_x = self.cursor_x

        if self.cursor_x >= self.view_x + visible_width:
            self.view_x = self.cursor_x - visible_width + 1

        if self.cursor_y < self.view_y:
            self.view_y = self.cursor_y

        if self.cursor_y >= self.view_y + visible_height:
            self.view_y = self.cursor_y - visible_height + 1

        max_view_x = max(0, self.width - visible_width)
        max_view_y = max(0, self.height - visible_height)

        self.view_x = min(self.view_x, max_view_x)
        self.view_y = min(self.view_y, max_view_y)

    def draw_palette_bar(self, screen, row, cols):
        x = 0

        for index in range(PALETTE_SIZE):
            if x + 4 > cols:
                break

            attributes = curses.color_pair(index + 1) | curses.A_BOLD

            if index == self.selected_index:
                attributes |= curses.A_REVERSE

            try:
                screen.addstr(row, x, f" {index:X}  ", attributes)
            except curses.error:
                pass

            x += 4

    def draw(self, screen):
        screen.erase()

        rows, cols = screen.getmaxyx()

        if rows < 5 or cols < 8:
            screen.addstr(0, 0, "Terminal too small")
            screen.refresh()
            return

        self.ensure_visible(rows, cols)

        visible_width = max(1, cols // 2)
        visible_height = max(1, rows - 3)

        end_x = min(self.width, self.view_x + visible_width)
        end_y = min(self.height, self.view_y + visible_height)

        for image_y in range(self.view_y, end_y):
            screen_y = image_y - self.view_y

            for image_x in range(self.view_x, end_x):
                screen_x = (image_x - self.view_x) * 2
                index = self.pixel(image_x, image_y)

                if image_x == self.cursor_x and image_y == self.cursor_y:
                    character = SELECTED_BLOCK
                    attributes = curses.color_pair(index + 1)
                else:
                    character = BLOCK
                    attributes = curses.color_pair(index + 1)

                try:
                    screen.addstr(
                        screen_y,
                        screen_x,
                        character,
                        attributes,
                    )
                except curses.error:
                    pass

        current_pixel = self.pixel(self.cursor_x, self.cursor_y)

        status = (
            f"Pos {self.cursor_x},{self.cursor_y}  "
            f"Pixel {current_pixel:X}  "
            f"Paint {self.selected_index:X}  "
            f"View {self.view_x},{self.view_y}  "
            f"Size {self.width}x{self.height}"
        )

        if self.modified or self.palette_modified:
            status += "  *MODIFIED*"

        status_row = rows - 3
        palette_row = rows - 2
        help_row = rows - 1

        try:
            screen.addstr(status_row, 0, status[:cols - 1])
        except curses.error:
            pass

        self.draw_palette_bar(screen, palette_row, cols)

        help_text = (
            "Arrows/HJKL move | 0-F select | Space paint | "
            "Enter/P edit RGB | S save | Q quit"
        )

        try:
            screen.addstr(help_row, 0, help_text[:cols - 1])
        except curses.error:
            pass

        screen.refresh()

    def save(self):
        save_4bpp_image(
            self.image_path,
            self.width,
            self.height,
            self.pixels,
            self.palette,
        )

        write_jasc_palette(self.palette_path, self.palette)

        self.modified = False
        self.palette_modified = False

    def edit_palette(self, screen):
        palette_index = self.selected_index
        original_color = self.palette[palette_index]

        # Empty strings let the first typed number replace the old value.
        fields = ["", "", ""]
        field_index = 0

        def current_value():
            values = []

            for position, field in enumerate(fields):
                if field == "":
                    values.append(original_color[position])
                    continue

                try:
                    values.append(clamp(int(field)))
                except ValueError:
                    values.append(original_color[position])

            return tuple(values)

        def apply_preview():
            self.palette[palette_index] = current_value()
            self.refresh_color_pairs()

        def draw_dialog():
            self.draw(screen)

            rows, cols = screen.getmaxyx()

            dialog_width = min(48, max(30, cols - 4))
            dialog_height = 13

            dialog_y = max(1, (rows - dialog_height) // 2)
            dialog_x = max(1, (cols - dialog_width) // 2)

            # Fill the dialog background.
            for row in range(dialog_y, dialog_y + dialog_height):
                try:
                    screen.addstr(
                        row,
                        dialog_x,
                        " " * min(
                            dialog_width,
                            cols - dialog_x - 1,
                        ),
                        curses.A_REVERSE,
                    )
                except curses.error:
                    pass

            red, green, blue = current_value()

            try:
                screen.addstr(
                    dialog_y + 1,
                    dialog_x + 2,
                    f"Edit palette entry {palette_index:X}",
                    curses.A_REVERSE | curses.A_BOLD,
                )

                screen.addstr(
                    dialog_y + 3,
                    dialog_x + 2,
                    "RGB values:",
                    curses.A_REVERSE,
                )

                labels = ("R", "G", "B")
                values = (red, green, blue)

                for position, (label, value) in enumerate(
                    zip(labels, values)
                ):
                    x = dialog_x + 2 + position * 11

                    if position == field_index:
                        attributes = (
                            curses.A_REVERSE
                            | curses.A_BOLD
                            | curses.A_UNDERLINE
                        )
                    else:
                        attributes = curses.A_REVERSE

                    visible_value = fields[position]

                    if visible_value == "":
                        visible_value = str(value)

                    screen.addstr(
                        dialog_y + 5,
                        x,
                        f"{label}: {visible_value:<3}",
                        attributes,
                    )

                # Preview the edited color inside the dialog.
                preview_color = curses.color_pair(palette_index + 1)

                screen.addstr(
                    dialog_y + 7,
                    dialog_x + 2,
                    "Preview:",
                    curses.A_REVERSE,
                )

                screen.addstr(
                    dialog_y + 7,
                    dialog_x + 12,
                    "████████",
                    preview_color | curses.A_BOLD,
                )

                screen.addstr(
                    dialog_y + 9,
                    dialog_x + 2,
                    "Left/Right: field    Up/Down: change",
                    curses.A_REVERSE,
                )

                screen.addstr(
                    dialog_y + 10,
                    dialog_x + 2,
                    "Type digits    Enter: apply    Esc: cancel",
                    curses.A_REVERSE,
                )

            except curses.error:
                pass

            screen.refresh()

        while True:
            draw_dialog()
            key = screen.getch()

            if key == 27:
                # Restore the original palette entry.
                self.palette[palette_index] = original_color
                self.refresh_color_pairs()
                return

            if key in (curses.KEY_LEFT, ord("h")):
                field_index = max(0, field_index - 1)
                continue

            if key in (
                curses.KEY_RIGHT,
                ord("l"),
                9,
            ):
                field_index = min(2, field_index + 1)
                continue

            if key in (curses.KEY_UP, ord("k")):
                value = current_value()[field_index]
                value = clamp(value + 1)
                fields[field_index] = str(value)
                apply_preview()
                continue

            if key in (curses.KEY_DOWN, ord("j")):
                value = current_value()[field_index]
                value = clamp(value - 1)
                fields[field_index] = str(value)
                apply_preview()
                continue

            if key in (
                curses.KEY_BACKSPACE,
                127,
                8,
            ):
                fields[field_index] = fields[field_index][:-1]
                apply_preview()
                continue

            if key in (
                curses.KEY_ENTER,
                10,
                13,
            ):
                self.palette_modified = True
                self.refresh_color_pairs()
                return

            if ord("0") <= key <= ord("9"):
                digit = chr(key)

                if len(fields[field_index]) >= 3:
                    fields[field_index] = ""

                fields[field_index] += digit

                # Keep values valid while typing.
                if int(fields[field_index]) > 255:
                    fields[field_index] = "255"

                apply_preview()

    def run(self, screen):
        curses.curs_set(0)
        screen.keypad(True)
        screen.timeout(-1)

        if not curses.has_colors():
            raise RuntimeError("This terminal does not support colors")

        curses.start_color()
        curses.use_default_colors()

        if curses.COLORS < 16:
            raise RuntimeError("This terminal needs at least 16 colors")

        self.refresh_color_pairs()

        while True:
            self.draw(screen)
            key = screen.getch()

            if key in (
                ord("q"),
                ord("Q"),
                27,
            ):
                if self.modified or self.palette_modified:
                    rows, cols = screen.getmaxyx()

                    message = (
                        "Unsaved changes. "
                        "S = save, Q = quit, any other key = cancel"
                    )

                    try:
                        screen.addstr(
                            0,
                            0,
                            message[:cols - 1],
                            curses.A_BOLD,
                        )
                    except curses.error:
                        pass

                    screen.refresh()
                    confirmation = screen.getch()

                    if confirmation in (ord("s"), ord("S")):
                        self.save()
                    elif confirmation not in (
                        ord("q"),
                        ord("Q"),
                    ):
                        continue

                break

            if key in (
                curses.KEY_LEFT,
                ord("h"),
            ):
                self.move(-1, 0)

            elif key in (
                curses.KEY_RIGHT,
                ord("l"),
            ):
                self.move(1, 0)

            elif key in (
                curses.KEY_UP,
                ord("k"),
            ):
                self.move(0, -1)

            elif key in (
                curses.KEY_DOWN,
                ord("j"),
            ):
                self.move(0, 1)

            elif key in (
                ord(" "),
                ord("e"),
                ord("E"),
            ):
                self.set_pixel(
                    self.cursor_x,
                    self.cursor_y,
                    self.selected_index,
                )

            elif key in (
                ord("p"),
                ord("P"),
                curses.KEY_ENTER,
                10,
                13,
            ):
                self.edit_palette(screen)

            elif key in (
                ord("s"),
                ord("S"),
            ):
                self.save()

            else:
                palette_index = key_to_index(key)

                if palette_index is not None:
                    self.selected_index = palette_index


def main():
    parser = argparse.ArgumentParser(
        description="Terminal 4bpp image and JASC palette editor"
    )

    parser.add_argument(
        "image",
        help="Indexed image to edit",
    )

    parser.add_argument(
        "palette",
        help="JASC .pal file to edit",
    )

    args = parser.parse_args()

    if not os.path.exists(args.image):
        parser.error(f"Image does not exist: {args.image}")

    if not os.path.exists(args.palette):
        parser.error(f"Palette does not exist: {args.palette}")

    try:
        editor = Editor(args.image, args.palette)
        curses.wrapper(editor.run)

    except KeyboardInterrupt:
        print("\nExited.")

    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()