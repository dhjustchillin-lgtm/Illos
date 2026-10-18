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
CELL_WIDTH = 2

ESC = "\x1b"
RESET = f"{ESC}[0m"
BOLD = f"{ESC}[1m"
REVERSE = f"{ESC}[7m"
UNDERLINE = f"{ESC}[4m"
HIDE_CURSOR = f"{ESC}[?25l"
SHOW_CURSOR = f"{ESC}[?25h"
CLEAR_SCREEN = f"{ESC}[2J"
CURSOR_HOME = f"{ESC}[H"


def clamp(value, low=0, high=255):
    return max(low, min(high, value))


def cursor_position(row, column):
    return f"{ESC}[{row + 1};{column + 1}H"


def foreground_rgb(rgb):
    red, green, blue = rgb
    return f"{ESC}[38;2;{red};{green};{blue}m"


def colored_text(text, rgb, attributes=""):
    return (
        attributes
        + foreground_rgb(rgb)
        + text
        + RESET
    )


def write_terminal(text):
    data = text.encode("utf-8")

    try:
        os.write(sys.stdout.fileno(), data)
    except OSError:
        sys.stdout.write(text)
        sys.stdout.flush()


def read_jasc_palette(path):
    lines = [
        line.strip()
        for line in Path(path)
        .read_text(encoding="utf-8")
        .splitlines()
        if line.strip()
    ]

    if len(lines) < 3:
        raise ValueError(
            f"{path} is not a valid JASC-PAL file"
        )

    if lines[0].upper() != "JASC-PAL":
        raise ValueError(
            f"{path} is not a valid JASC-PAL file"
        )

    try:
        color_count = int(lines[2])
    except ValueError as exc:
        raise ValueError(
            "Invalid JASC palette color count"
        ) from exc

    colors = []

    for line in lines[3:3 + color_count]:
        parts = line.split()

        if len(parts) != 3:
            raise ValueError(
                f"Invalid palette line: {line}"
            )

        try:
            rgb = tuple(
                clamp(int(value))
                for value in parts
            )
        except ValueError as exc:
            raise ValueError(
                f"Invalid palette line: {line}"
            ) from exc

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

    for red, green, blue in palette[:PALETTE_SIZE]:
        lines.append(f"{red} {green} {blue}")

    Path(path).write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def load_4bpp_image(path):
    image = Image.open(path).convert("P")

    pixels = [
        value & 0x0F
        for value in image.getdata()
    ]

    return image.width, image.height, pixels


def save_4bpp_image(path, width, height, pixels, palette):
    image = Image.new("P", (width, height))

    image.putdata([
        value & 0x0F
        for value in pixels
    ])

    flat_palette = []

    for red, green, blue in palette[:PALETTE_SIZE]:
        flat_palette.extend(
            (red, green, blue)
        )

    flat_palette.extend(
        [0] * (768 - len(flat_palette))
    )

    image.putpalette(flat_palette)
    image.save(path)


def key_to_index(key):
    if ord("0") <= key <= ord("9"):
        return key - ord("0")

    if ord("a") <= key <= ord("f"):
        return key - ord("a") + 10

    if ord("A") <= key <= ord("F"):
        return key - ord("A") + 10

    return None


def parse_positive_integer(text):
    try:
        value = int(text)
    except ValueError:
        return None

    if value < 1:
        return None

    return value


class Editor:
    def __init__(self, image_path, palette_path):
        self.image_path = image_path
        self.palette_path = palette_path

        (
            self.width,
            self.height,
            self.pixels,
        ) = load_4bpp_image(image_path)

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
            min(
                self.width - 1,
                self.cursor_x + dx,
            ),
        )

        self.cursor_y = max(
            0,
            min(
                self.height - 1,
                self.cursor_y + dy,
            ),
        )

    def resize_canvas(self, new_width, new_height):
        old_width = self.width
        old_height = self.height
        old_pixels = self.pixels

        # Start the entire new canvas with palette index 0.
        new_pixels = [
            0
            for _ in range(new_width * new_height)
        ]

        # Copy the old image into the same top-left coordinates.
        copy_width = min(old_width, new_width)
        copy_height = min(old_height, new_height)

        for y in range(copy_height):
            old_start = y * old_width
            new_start = y * new_width

            new_pixels[
                new_start:new_start + copy_width
            ] = old_pixels[
                old_start:old_start + copy_width
            ]

        self.width = new_width
        self.height = new_height
        self.pixels = new_pixels

        self.cursor_x = min(
            self.cursor_x,
            self.width - 1,
        )

        self.cursor_y = min(
            self.cursor_y,
            self.height - 1,
        )

        self.view_x = 0
        self.view_y = 0
        self.modified = True

    def ensure_visible(self, rows, cols):
        visible_width = max(
            1,
            cols // CELL_WIDTH,
        )

        visible_height = max(
            1,
            rows - 3,
        )

        if self.cursor_x < self.view_x:
            self.view_x = self.cursor_x

        if self.cursor_x >= (
            self.view_x + visible_width
        ):
            self.view_x = (
                self.cursor_x
                - visible_width
                + 1
            )

        if self.cursor_y < self.view_y:
            self.view_y = self.cursor_y

        if self.cursor_y >= (
            self.view_y + visible_height
        ):
            self.view_y = (
                self.cursor_y
                - visible_height
                + 1
            )

        max_view_x = max(
            0,
            self.width - visible_width,
        )

        max_view_y = max(
            0,
            self.height - visible_height,
        )

        self.view_x = min(
            self.view_x,
            max_view_x,
        )

        self.view_y = min(
            self.view_y,
            max_view_y,
        )

    def append_text(
        self,
        output,
        row,
        column,
        text,
        attributes="",
    ):
        output.append(
            cursor_position(row, column)
        )
        output.append(attributes)
        output.append(text)
        output.append(RESET)

    def append_palette_bar(self, output, row, cols):
        column = 0

        for index in range(PALETTE_SIZE):
            label = f" {index:X}  "

            if column + len(label) > cols:
                break

            attributes = BOLD

            if index == self.selected_index:
                attributes += REVERSE

            output.append(
                cursor_position(row, column)
            )

            output.append(
                colored_text(
                    label,
                    self.palette[index],
                    attributes,
                )
            )

            column += len(label)

    def draw(self, screen):
        rows, cols = screen.getmaxyx()

        output = [
            HIDE_CURSOR,
            CLEAR_SCREEN,
            CURSOR_HOME,
        ]

        if rows < 5 or cols < 8:
            self.append_text(
                output,
                0,
                0,
                "Terminal too small",
                BOLD,
            )

            write_terminal("".join(output))
            return

        self.ensure_visible(rows, cols)

        visible_width = max(
            1,
            cols // CELL_WIDTH,
        )

        visible_height = max(
            1,
            rows - 3,
        )

        end_x = min(
            self.width,
            self.view_x + visible_width,
        )

        end_y = min(
            self.height,
            self.view_y + visible_height,
        )

        for image_y in range(
            self.view_y,
            end_y,
        ):
            screen_y = image_y - self.view_y

            output.append(
                cursor_position(screen_y, 0)
            )

            for image_x in range(
                self.view_x,
                end_x,
            ):
                index = self.pixel(
                    image_x,
                    image_y,
                )

                rgb = self.palette[index]

                is_cursor = (
                    image_x == self.cursor_x
                    and image_y == self.cursor_y
                )

                if is_cursor:
                    output.append(
                        colored_text(
                            SELECTED_BLOCK,
                            rgb,
                            REVERSE,
                        )
                    )
                else:
                    output.append(
                        colored_text(
                            BLOCK,
                            rgb,
                        )
                    )

        current_pixel = self.pixel(
            self.cursor_x,
            self.cursor_y,
        )

        status = (
            f"Pos {self.cursor_x},{self.cursor_y}  "
            f"Pixel {current_pixel:X}  "
            f"Paint {self.selected_index:X}  "
            f"Size {self.width}x{self.height}"
        )

        if self.modified or self.palette_modified:
            status += "  *MODIFIED*"

        self.append_text(
            output,
            rows - 3,
            0,
            status[:max(0, cols - 1)],
            BOLD,
        )

        self.append_palette_bar(
            output,
            rows - 2,
            cols,
        )

        help_text = (
            "Arrows/HJKL move | 0-F select | "
            "Space paint | P palette | R resize | "
            "S save | Q quit"
        )

        self.append_text(
            output,
            rows - 1,
            0,
            help_text[:max(0, cols - 1)],
        )

        output.append(RESET)
        output.append(HIDE_CURSOR)

        write_terminal("".join(output))

    def save(self):
        save_4bpp_image(
            self.image_path,
            self.width,
            self.height,
            self.pixels,
            self.palette,
        )

        write_jasc_palette(
            self.palette_path,
            self.palette,
        )

        self.modified = False
        self.palette_modified = False

    def draw_dialog_background(
        self,
        output,
        rows,
        cols,
        title,
        dialog_width,
        dialog_height,
    ):
        dialog_width = min(
            dialog_width,
            max(30, cols - 4),
        )

        dialog_y = max(
            1,
            (rows - dialog_height) // 2,
        )

        dialog_x = max(
            1,
            (cols - dialog_width) // 2,
        )

        fill_width = min(
            dialog_width,
            max(1, cols - dialog_x - 1),
        )

        for row in range(
            dialog_y,
            dialog_y + dialog_height,
        ):
            self.append_text(
                output,
                row,
                dialog_x,
                " " * fill_width,
                REVERSE,
            )

        self.append_text(
            output,
            dialog_y + 1,
            dialog_x + 2,
            title,
            REVERSE + BOLD,
        )

        return dialog_y, dialog_x

    def resize_dialog(self, screen):
        width_text = str(self.width)
        height_text = str(self.height)
        field_index = 0

        while True:
            self.draw(screen)

            rows, cols = screen.getmaxyx()
            output = []

            dialog_y, dialog_x = (
                self.draw_dialog_background(
                    output,
                    rows,
                    cols,
                    "Resize canvas",
                    58,
                    12,
                )
            )

            width_attributes = REVERSE
            height_attributes = REVERSE

            if field_index == 0:
                width_attributes += BOLD + UNDERLINE
            else:
                height_attributes += BOLD + UNDERLINE

            self.append_text(
                output,
                dialog_y + 3,
                dialog_x + 2,
                f"Width:  {width_text}",
                width_attributes,
            )

            self.append_text(
                output,
                dialog_y + 4,
                dialog_x + 2,
                f"Height: {height_text}",
                height_attributes,
            )

            self.append_text(
                output,
                dialog_y + 6,
                dialog_x + 2,
                "New cells are filled with palette index 0.",
                REVERSE,
            )

            self.append_text(
                output,
                dialog_y + 8,
                dialog_x + 2,
                "Left/Right or Tab: field",
                REVERSE,
            )

            self.append_text(
                output,
                dialog_y + 9,
                dialog_x + 2,
                "Type digits    Enter: apply",
                REVERSE,
            )

            self.append_text(
                output,
                dialog_y + 10,
                dialog_x + 2,
                "Esc: cancel",
                REVERSE,
            )

            output.append(HIDE_CURSOR)
            write_terminal("".join(output))

            key = screen.getch()

            if key == 27:
                return

            if key in (
                curses.KEY_LEFT,
                curses.KEY_RIGHT,
                9,
            ):
                field_index = 1 - field_index
                continue

            if key in (
                curses.KEY_UP,
                curses.KEY_DOWN,
            ):
                field_index = 1 - field_index
                continue

            if key in (
                curses.KEY_BACKSPACE,
                127,
                8,
            ):
                if field_index == 0:
                    width_text = width_text[:-1]
                else:
                    height_text = height_text[:-1]

                continue

            if key in (
                curses.KEY_ENTER,
                10,
                13,
            ):
                new_width = parse_positive_integer(
                    width_text
                )

                new_height = parse_positive_integer(
                    height_text
                )

                if new_width is None:
                    continue

                if new_height is None:
                    continue

                self.resize_canvas(
                    new_width,
                    new_height,
                )

                return

            if ord("0") <= key <= ord("9"):
                digit = chr(key)

                if field_index == 0:
                    if len(width_text) < 6:
                        width_text += digit
                else:
                    if len(height_text) < 6:
                        height_text += digit

    def palette_dialog(self, screen):
        palette_index = self.selected_index
        original_color = self.palette[palette_index]

        fields = ["", "", ""]
        field_index = 0

        while True:
            self.draw(screen)

            rows, cols = screen.getmaxyx()
            output = []

            dialog_y, dialog_x = (
                self.draw_dialog_background(
                    output,
                    rows,
                    cols,
                    f"Edit palette entry {palette_index:X}",
                    58,
                    13,
                )
            )

            values = []

            for position, field in enumerate(fields):
                if field == "":
                    values.append(original_color[position])
                    continue

                try:
                    values.append(
                        clamp(int(field))
                    )
                except ValueError:
                    values.append(original_color[position])

            labels = ("R", "G", "B")

            for position, (label, value) in enumerate(
                zip(labels, values)
            ):
                attributes = REVERSE

                if position == field_index:
                    attributes += BOLD + UNDERLINE

                visible_value = fields[position]

                if visible_value == "":
                    visible_value = str(value)

                self.append_text(
                    output,
                    dialog_y + 4,
                    dialog_x + 2 + position * 11,
                    f"{label}: {visible_value:<3}",
                    attributes,
                )

            self.append_text(
                output,
                dialog_y + 6,
                dialog_x + 2,
                "Preview:",
                REVERSE,
            )

            output.append(
                cursor_position(
                    dialog_y + 6,
                    dialog_x + 12,
                )
            )

            output.append(
                colored_text(
                    "████████",
                    tuple(values),
                    BOLD,
                )
            )

            self.append_text(
                output,
                dialog_y + 8,
                dialog_x + 2,
                "Left/Right or Tab: field",
                REVERSE,
            )

            self.append_text(
                output,
                dialog_y + 9,
                dialog_x + 2,
                "Type digits    Enter: apply",
                REVERSE,
            )

            self.append_text(
                output,
                dialog_y + 10,
                dialog_x + 2,
                "Esc: cancel",
                REVERSE,
            )

            output.append(HIDE_CURSOR)
            write_terminal("".join(output))

            key = screen.getch()

            if key == 27:
                self.palette[palette_index] = original_color
                return

            if key in (
                curses.KEY_LEFT,
                curses.KEY_RIGHT,
                9,
            ):
                field_index = (
                    field_index + 1
                ) % 3
                continue

            if key in (
                curses.KEY_UP,
                curses.KEY_DOWN,
            ):
                value = values[field_index]

                if key == curses.KEY_UP:
                    value = clamp(value + 1)
                else:
                    value = clamp(value - 1)

                fields[field_index] = str(value)

            elif key in (
                curses.KEY_BACKSPACE,
                127,
                8,
            ):
                fields[field_index] = (
                    fields[field_index][:-1]
                )

            elif key in (
                curses.KEY_ENTER,
                10,
                13,
            ):
                self.palette[palette_index] = (
                    tuple(
                        clamp(int(field))
                        if field
                        else original_color[position]
                        for position, field in enumerate(fields)
                    )
                )

                self.palette_modified = True
                return

            elif ord("0") <= key <= ord("9"):
                digit = chr(key)

                if len(fields[field_index]) >= 3:
                    fields[field_index] = ""

                fields[field_index] += digit

                if int(fields[field_index]) > 255:
                    fields[field_index] = "255"

            self.palette[palette_index] = (
                tuple(
                    clamp(int(field))
                    if field
                    else original_color[position]
                    for position, field in enumerate(fields)
                )
            )

            self.palette_modified = True

    def confirm_quit(self, screen):
        rows, cols = screen.getmaxyx()

        output = []

        message = (
            "Unsaved changes. "
            "S = save, Q = quit, "
            "any other key = cancel"
        )

        self.append_text(
            output,
            0,
            0,
            message[:max(0, cols - 1)],
            BOLD,
        )

        write_terminal("".join(output))

        confirmation = screen.getch()

        if confirmation in (
            ord("s"),
            ord("S"),
        ):
            self.save()
            return True

        if confirmation in (
            ord("q"),
            ord("Q"),
        ):
            return True

        return False

    def run(self, screen):
        curses.curs_set(0)
        screen.keypad(True)
        screen.timeout(-1)

        # Curses is used for input and dimensions only.
        # All visible output is sent directly as ANSI true color.
        screen.clearok(False)
        screen.leaveok(True)

        screen.erase()
        screen.refresh()

        while True:
            self.draw(screen)
            key = screen.getch()

            if key in (
                ord("q"),
                ord("Q"),
                27,
            ):
                if self.modified or self.palette_modified:
                    if not self.confirm_quit(screen):
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
                self.palette_dialog(screen)

            elif key in (
                ord("r"),
                ord("R"),
            ):
                self.resize_dialog(screen)

            elif key in (
                ord("s"),
                ord("S"),
            ):
                self.save()

            else:
                palette_index = key_to_index(key)

                if palette_index is not None:
                    self.selected_index = palette_index


def restore_terminal():
    write_terminal(
        RESET
        + SHOW_CURSOR
        + CLEAR_SCREEN
        + CURSOR_HOME
    )


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Terminal 4bpp image and JASC palette editor"
        )
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
        parser.error(
            f"Image does not exist: {args.image}"
        )

    if not os.path.exists(args.palette):
        parser.error(
            f"Palette does not exist: {args.palette}"
        )

    try:
        editor = Editor(
            args.image,
            args.palette,
        )

        curses.wrapper(editor.run)

    except KeyboardInterrupt:
        pass

    except Exception as exc:
        print(
            f"Error: {exc}",
            file=sys.stderr,
        )
        sys.exit(1)

    finally:
        restore_terminal()


if __name__ == "__main__":
    main()