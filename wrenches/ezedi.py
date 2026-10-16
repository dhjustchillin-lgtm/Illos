#!/usr/bin/env python3
"""Pygame 16-color indexed image and JASC palette editor.

Usage:
    python ezedi_pygame.py image.png palette.pal

Requires:
    python -m pip install pygame pillow
"""
import argparse
import os
import sys
from pathlib import Path

import pygame
from PIL import Image

PALETTE_SIZE = 16
WINDOW_SIZE = (1024, 768)
PANEL_WIDTH = 230
STATUS_HEIGHT = 56
DEFAULT_ZOOM = 16
MIN_ZOOM = 3
MAX_ZOOM = 48
BG = (35, 37, 42)
PANEL_BG = (48, 51, 58)
TEXT = (235, 237, 240)
MUTED = (170, 176, 186)
ACCENT = (90, 170, 255)


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
            colors.append(tuple(clamp(int(value)) for value in parts))
        except ValueError as exc:
            raise ValueError(f"Invalid palette line: {line}") from exc
    while len(colors) < PALETTE_SIZE:
        colors.append((0, 0, 0))
    return colors[:PALETTE_SIZE]


def write_jasc_palette(path, palette):
    lines = ["JASC-PAL", "0100", str(PALETTE_SIZE)]
    lines.extend(f"{r} {g} {b}" for r, g, b in palette[:PALETTE_SIZE])
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


def load_4bpp_image(path):
    image = Image.open(path).convert("P")
    return image.width, image.height, [int(value) & 0x0F for value in image.getdata()]


def save_4bpp_image(path, width, height, pixels, palette):
    image = Image.new("P", (width, height))
    image.putdata([value & 0x0F for value in pixels])
    flat_palette = []
    for r, g, b in palette[:PALETTE_SIZE]:
        flat_palette.extend((r, g, b))
    flat_palette.extend([0] * (768 - len(flat_palette)))
    image.putpalette(flat_palette)
    image.save(path)


def rgb_hex(rgb):
    return "#{:02X}{:02X}{:02X}".format(*rgb)


class Editor:
    def __init__(self, image_path, palette_path):
        pygame.init()
        pygame.display.set_caption("EZEdi — 16-color indexed image editor")
        self.screen = pygame.display.set_mode(WINDOW_SIZE, pygame.RESIZABLE)
        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont("dejavusans", 16)
        self.small_font = pygame.font.SysFont("dejavusans", 13)
        self.image_path = image_path
        self.palette_path = palette_path
        self.width, self.height, self.pixels = load_4bpp_image(image_path)
        self.palette = read_jasc_palette(palette_path)
        self.cursor_x = 0
        self.cursor_y = 0
        self.selected_index = 0
        self.zoom = DEFAULT_ZOOM
        self.view_x = 0
        self.view_y = 0
        self.modified = False
        self.palette_modified = False
        self.running = True
        self.message = "Ready"
        self.message_until = 0
        self.dialog = None
        self.rgb_fields = []
        self.rgb_field = 0
        self.rgb_text = ["", "", ""]
        self.rgb_original = (0, 0, 0)
        self.dragging = False
        self.update_title()

    def update_title(self):
        suffix = " *" if self.modified or self.palette_modified else ""
        pygame.display.set_caption(
            f"EZEdi — {os.path.basename(self.image_path)}{suffix}"
        )

    def notify(self, text):
        self.message = text
        self.message_until = pygame.time.get_ticks() + 2500

    def pixel(self, x, y):
        return self.pixels[y * self.width + x]

    def set_pixel(self, x, y, value):
        offset = y * self.width + x
        value &= 0x0F
        if self.pixels[offset] != value:
            self.pixels[offset] = value
            self.modified = True
            self.update_title()

    def move(self, dx, dy):
        self.cursor_x = max(0, min(self.width - 1, self.cursor_x + dx))
        self.cursor_y = max(0, min(self.height - 1, self.cursor_y + dy))
        self.keep_cursor_visible()

    def keep_cursor_visible(self):
        view_w, view_h = self.canvas_size()
        if self.cursor_x < self.view_x:
            self.view_x = self.cursor_x
        elif self.cursor_x >= self.view_x + view_w // self.zoom:
            self.view_x = self.cursor_x - view_w // self.zoom + 1
        if self.cursor_y < self.view_y:
            self.view_y = self.cursor_y
        elif self.cursor_y >= self.view_y + view_h // self.zoom:
            self.view_y = self.cursor_y - view_h // self.zoom + 1
        self.clamp_view()

    def canvas_size(self):
        w, h = self.screen.get_size()
        return max(1, w - PANEL_WIDTH), max(1, h - STATUS_HEIGHT)

    def clamp_view(self):
        view_w, view_h = self.canvas_size()
        cols = max(1, view_w // self.zoom)
        rows = max(1, view_h // self.zoom)
        self.view_x = max(0, min(max(0, self.width - cols), self.view_x))
        self.view_y = max(0, min(max(0, self.height - rows), self.view_y))

    def pixel_at(self, pos):
        x, y = pos
        view_w, view_h = self.canvas_size()
        if x < 0 or y < 0 or x >= view_w or y >= view_h:
            return None
        ix = self.view_x + x // self.zoom
        iy = self.view_y + y // self.zoom
        if 0 <= ix < self.width and 0 <= iy < self.height:
            return ix, iy
        return None

    def paint_at(self, pos):
        point = self.pixel_at(pos)
        if point is not None:
            self.cursor_x, self.cursor_y = point
            self.set_pixel(*point, self.selected_index)

    def save(self):
        try:
            save_4bpp_image(
                self.image_path, self.width, self.height, self.pixels, self.palette
            )
            write_jasc_palette(self.palette_path, self.palette)
            self.modified = False
            self.palette_modified = False
            self.update_title()
            self.notify("Saved image and palette")
        except Exception as exc:
            self.notify(f"Save failed: {exc}")

    def open_rgb_dialog(self):
        self.dialog = "rgb"
        self.rgb_original = self.palette[self.selected_index]
        self.rgb_text = [str(v) for v in self.rgb_original]
        self.rgb_field = 0

    def rgb_values(self):
        values = []
        for i, text in enumerate(self.rgb_text):
            try:
                values.append(clamp(int(text)) if text else self.rgb_original[i])
            except ValueError:
                values.append(self.rgb_original[i])
        return tuple(values)

    def apply_rgb_dialog(self):
        self.palette[self.selected_index] = self.rgb_values()
        self.palette_modified = True
        self.dialog = None
        self.update_title()
        self.notify(f"Palette {self.selected_index:X} updated")

    def cancel_rgb_dialog(self):
        self.palette[self.selected_index] = self.rgb_original
        self.dialog = None

    def quit(self):
        if self.modified or self.palette_modified:
            self.dialog = "quit"
        else:
            self.running = False

    def handle_key(self, event):
        if self.dialog == "rgb":
            if event.key == pygame.K_ESCAPE:
                self.cancel_rgb_dialog()
            elif event.key in (pygame.K_RETURN, pygame.K_KP_ENTER):
                self.apply_rgb_dialog()
            elif event.key == pygame.K_TAB or event.key == pygame.K_RIGHT:
                self.rgb_field = (self.rgb_field + 1) % 3
            elif event.key == pygame.K_LEFT:
                self.rgb_field = (self.rgb_field - 1) % 3
            elif event.key == pygame.K_BACKSPACE:
                self.rgb_text[self.rgb_field] = self.rgb_text[self.rgb_field][:-1]
            elif event.unicode.isdigit():
                current = self.rgb_text[self.rgb_field]
                self.rgb_text[self.rgb_field] = ("" if len(current) >= 3 else current) + event.unicode
                if int(self.rgb_text[self.rgb_field] or "0") > 255:
                    self.rgb_text[self.rgb_field] = "255"
            return

        if self.dialog == "quit":
            if event.key == pygame.K_ESCAPE:
                self.dialog = None
            elif event.key in (pygame.K_y, pygame.K_s, pygame.K_RETURN):
                self.save()
                if not self.modified and not self.palette_modified:
                    self.running = False
                self.dialog = None
            elif event.key == pygame.K_n:
                self.running = False
            return

        if event.key == pygame.K_ESCAPE or event.key == pygame.K_q:
            self.quit()
        elif event.key == pygame.K_s and (event.mod & pygame.KMOD_CTRL or event.mod == 0):
            self.save()
        elif event.key in (pygame.K_LEFT, pygame.K_a, pygame.K_h):
            self.move(-1, 0)
        elif event.key in (pygame.K_RIGHT, pygame.K_d, pygame.K_l):
            self.move(1, 0)
        elif event.key in (pygame.K_UP, pygame.K_w, pygame.K_k):
            self.move(0, -1)
        elif event.key in (pygame.K_DOWN, pygame.K_s, pygame.K_j):
            self.move(0, 1)
        elif event.key == pygame.K_SPACE or event.key == pygame.K_e:
            self.set_pixel(self.cursor_x, self.cursor_y, self.selected_index)
        elif event.key in (pygame.K_RETURN, pygame.K_p):
            self.open_rgb_dialog()
        elif event.key in (pygame.K_EQUALS, pygame.K_PLUS, pygame.K_KP_PLUS):
            self.zoom = min(MAX_ZOOM, self.zoom + 2)
            self.keep_cursor_visible()
        elif event.key in (pygame.K_MINUS, pygame.K_KP_MINUS):
            self.zoom = max(MIN_ZOOM, self.zoom - 2)
            self.keep_cursor_visible()
        elif event.unicode and event.unicode.lower() in "0123456789abcdef":
            self.selected_index = int(event.unicode, 16)

    def palette_rect(self, index):
        col, row = index % 4, index // 4
        x = self.screen.get_width() - PANEL_WIDTH + 14 + col * 52
        y = 100 + row * 48
        return pygame.Rect(x, y, 42, 32)

    def handle_mouse(self, event):
        mx, my = event.pos
        screen_w, screen_h = self.screen.get_size()
        canvas_w, canvas_h = self.canvas_size()

        if self.dialog == "rgb":
            if event.button == 1:
                # Click RGB field, Apply, or Cancel in the modal.
                box = self.dialog_rect()
                bx, by, bw, bh = box
                for i in range(3):
                    rect = pygame.Rect(bx + 24 + i * 94, by + 88, 76, 34)
                    if rect.collidepoint(mx, my):
                        self.rgb_field = i
                        return
                if pygame.Rect(bx + 130, by + 156, 80, 30).collidepoint(mx, my):
                    self.apply_rgb_dialog()
                elif pygame.Rect(bx + 220, by + 156, 80, 30).collidepoint(mx, my):
                    self.cancel_rgb_dialog()
            return

        if self.dialog == "quit":
            if event.button == 1:
                bx, by, bw, bh = self.dialog_rect()
                if pygame.Rect(bx + 24, by + 86, 100, 34).collidepoint(mx, my):
                    self.save()
                    if not self.modified and not self.palette_modified:
                        self.running = False
                    self.dialog = None
                elif pygame.Rect(bx + 136, by + 86, 100, 34).collidepoint(mx, my):
                    self.running = False
                elif pygame.Rect(bx + 248, by + 86, 100, 34).collidepoint(mx, my):
                    self.dialog = None
            return

        if event.button == 1 and mx < canvas_w and my < canvas_h:
            point = self.pixel_at(event.pos)
            if point:
                self.cursor_x, self.cursor_y = point
                self.set_pixel(*point, self.selected_index)
                self.dragging = True
            return
        if event.button == 3 and mx < canvas_w and my < canvas_h:
            point = self.pixel_at(event.pos)
            if point:
                self.cursor_x, self.cursor_y = point
                self.selected_index = self.pixel(*point)
            return
        if event.button == 1:
            # Palette swatches
            for i in range(PALETTE_SIZE):
                if self.palette_rect(i).collidepoint(mx, my):
                    self.selected_index = i
                    return
            # Panel buttons
            if pygame.Rect(screen_w - PANEL_WIDTH + 14, 42, 94, 32).collidepoint(mx, my):
                self.save()
            elif pygame.Rect(screen_w - PANEL_WIDTH + 116, 42, 96, 32).collidepoint(mx, my):
                self.open_rgb_dialog()
            elif pygame.Rect(screen_w - PANEL_WIDTH + 14, screen_h - STATUS_HEIGHT - 48, 92, 30).collidepoint(mx, my):
                self.zoom = max(MIN_ZOOM, self.zoom - 2)
                self.keep_cursor_visible()
            elif pygame.Rect(screen_w - PANEL_WIDTH + 116, screen_h - STATUS_HEIGHT - 48, 92, 30).collidepoint(mx, my):
                self.zoom = min(MAX_ZOOM, self.zoom + 2)
                self.keep_cursor_visible()

    def dialog_rect(self):
        w, h = self.screen.get_size()
        if self.dialog == "rgb":
            dw, dh = 360, 210
        else:
            dw, dh = 390, 145
        return ((w - dw) // 2, (h - dh) // 2, dw, dh)

    def draw_text(self, text, x, y, color=TEXT, font=None):
        surface = (font or self.font).render(str(text), True, color)
        self.screen.blit(surface, (x, y))
        return surface.get_rect(topleft=(x, y))

    def draw_button(self, rect, label):
        pygame.draw.rect(self.screen, (68, 74, 84), rect, border_radius=5)
        pygame.draw.rect(self.screen, (100, 108, 121), rect, 1, border_radius=5)
        text = self.font.render(label, True, TEXT)
        self.screen.blit(text, text.get_rect(center=rect.center))

    def draw(self):
        self.screen.fill(BG)
        screen_w, screen_h = self.screen.get_size()
        canvas_w, canvas_h = self.canvas_size()
        clip = pygame.Rect(0, 0, canvas_w, canvas_h)
        self.screen.set_clip(clip)

        c = self.zoom
        start_x, start_y = self.view_x, self.view_y
        end_x = min(self.width, start_x + canvas_w // c + 2)
        end_y = min(self.height, start_y + canvas_h // c + 2)
        for y in range(start_y, end_y):
            for x in range(start_x, end_x):
                rect = pygame.Rect((x - start_x) * c, (y - start_y) * c, c, c)
                pygame.draw.rect(self.screen, self.palette[self.pixel(x, y)], rect)
                if c >= 8:
                    pygame.draw.rect(self.screen, (58, 61, 67), rect, 1)
        cx = (self.cursor_x - start_x) * c
        cy = (self.cursor_y - start_y) * c
        pygame.draw.rect(self.screen, (255, 255, 255), (cx + 1, cy + 1, max(1, c - 2), max(1, c - 2)), 2)
        if c >= 12:
            pygame.draw.rect(self.screen, (0, 0, 0), (cx + 4, cy + 4, max(1, c - 8), max(1, c - 8)), 1)
        self.screen.set_clip(None)

        panel_x = screen_w - PANEL_WIDTH
        pygame.draw.rect(self.screen, PANEL_BG, (panel_x, 0, PANEL_WIDTH, screen_h))
        self.draw_text("EZEdi", panel_x + 14, 10, font=pygame.font.SysFont("dejavusans", 21, bold=True))
        self.draw_button(pygame.Rect(panel_x + 14, 42, 94, 32), "Save")
        self.draw_button(pygame.Rect(panel_x + 116, 42, 96, 32), "Edit RGB")
        self.draw_text("16-color palette", panel_x + 14, 82, MUTED, self.small_font)
        for i in range(PALETTE_SIZE):
            rect = self.palette_rect(i)
            pygame.draw.rect(self.screen, self.palette[i], rect)
            pygame.draw.rect(self.screen, ACCENT if i == self.selected_index else (105, 111, 122),
                             rect, 3 if i == self.selected_index else 1)
            self.draw_text(f"{i:X}", rect.x + 3, rect.y + 3,
                           (255, 255, 255) if sum(self.palette[i]) < 390 else (20, 20, 20),
                           self.small_font)
        selected = self.palette[self.selected_index]
        self.draw_text(f"Selected: {self.selected_index:X}", panel_x + 14, 302)
        self.draw_text(f"RGB {selected[0]}, {selected[1]}, {selected[2]}", panel_x + 14, 326, MUTED, self.small_font)
        pygame.draw.rect(self.screen, selected, (panel_x + 14, 350, 198, 26), border_radius=3)
        self.draw_text("Image", panel_x + 14, 390, MUTED, self.small_font)
        self.draw_text(f"{self.width} × {self.height}", panel_x + 14, 410)
        self.draw_text(f"Pixel {self.cursor_x}, {self.cursor_y}", panel_x + 14, 438, MUTED, self.small_font)
        self.draw_button(pygame.Rect(panel_x + 14, screen_h - STATUS_HEIGHT - 48, 92, 30), "Zoom −")
        self.draw_button(pygame.Rect(panel_x + 116, screen_h - STATUS_HEIGHT - 48, 92, 30), "Zoom +")
        self.draw_text(f"{self.zoom}px / pixel", panel_x + 14, screen_h - STATUS_HEIGHT - 14, MUTED, self.small_font)

        status_y = screen_h - STATUS_HEIGHT
        pygame.draw.rect(self.screen, (25, 27, 31), (0, status_y, screen_w, STATUS_HEIGHT))
        flags = []
        if self.modified:
            flags.append("IMAGE MODIFIED")
        if self.palette_modified:
            flags.append("PALETTE MODIFIED")
        status = "  ".join(flags) if flags else "Ready"
        self.draw_text(status, 12, status_y + 7, (255, 207, 102) if flags else MUTED, self.small_font)
        self.draw_text("Click/drag: paint  •  Right-click: pick  •  Arrows: move  •  Space: paint  •  0–F: color  •  Enter: RGB  •  Ctrl+S: save  •  Q: quit",
                       12, status_y + 29, TEXT, self.small_font)

        if self.dialog:
            self.draw_dialog()
        pygame.display.flip()

    def draw_dialog(self):
        overlay = pygame.Surface(self.screen.get_size(), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 155))
        self.screen.blit(overlay, (0, 0))
        bx, by, bw, bh = self.dialog_rect()
        pygame.draw.rect(self.screen, (49, 53, 61), (bx, by, bw, bh), border_radius=8)
        pygame.draw.rect(self.screen, (115, 124, 139), (bx, by, bw, bh), 1, border_radius=8)
        if self.dialog == "rgb":
            self.draw_text(f"Edit palette entry {self.selected_index:X}", bx + 20, by + 16,
                           font=pygame.font.SysFont("dejavusans", 18, bold=True))
            for i, label in enumerate(("Red", "Green", "Blue")):
                x = bx + 24 + i * 94
                self.draw_text(label, x, by + 60, MUTED, self.small_font)
                rect = pygame.Rect(x, by + 88, 76, 34)
                pygame.draw.rect(self.screen, (30, 32, 37), rect, border_radius=3)
                pygame.draw.rect(self.screen, ACCENT if i == self.rgb_field else (100, 108, 121), rect, 2, border_radius=3)
                value = self.rgb_text[i] or str(self.rgb_original[i])
                self.draw_text(value, x + 9, by + 95)
            pygame.draw.rect(self.screen, self.rgb_values(), (bx + 24, by + 132, 92, 12), border_radius=2)
            self.draw_button(pygame.Rect(bx + 130, by + 156, 80, 30), "Apply")
            self.draw_button(pygame.Rect(bx + 220, by + 156, 80, 30), "Cancel")
        elif self.dialog == "quit":
            self.draw_text("Unsaved changes", bx + 20, by + 18,
                           font=pygame.font.SysFont("dejavusans", 18, bold=True))
            self.draw_text("Save before quitting?", bx + 20, by + 48, MUTED)
            self.draw_button(pygame.Rect(bx + 24, by + 86, 100, 34), "Save")
            self.draw_button(pygame.Rect(bx + 136, by + 86, 100, 34), "Discard")
            self.draw_button(pygame.Rect(bx + 248, by + 86, 100, 34), "Cancel")

    def run(self):
        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.quit()
                elif event.type == pygame.KEYDOWN:
                    self.handle_key(event)
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    self.handle_mouse(event)
                elif event.type == pygame.MOUSEBUTTONUP:
                    self.dragging = False
                elif event.type == pygame.MOUSEMOTION and self.dragging and self.dialog is None:
                    self.paint_at(event.pos)
                elif event.type == pygame.MOUSEWHEEL:
                    mods = pygame.key.get_mods()
                    if mods & pygame.KMOD_CTRL:
                        self.zoom = max(MIN_ZOOM, min(MAX_ZOOM, self.zoom + (2 if event.y > 0 else -2)))
                        self.keep_cursor_visible()
            self.clamp_view()
            self.draw()
            self.clock.tick(60)
        pygame.quit()


def main():
    parser = argparse.ArgumentParser(description="Pygame 16-color indexed image and JASC palette editor")
    parser.add_argument("image", help="Indexed image to edit")
    parser.add_argument("palette", help="JASC .pal file to edit")
    args = parser.parse_args()
    if not os.path.exists(args.image):
        parser.error(f"Image does not exist: {args.image}")
    if not os.path.exists(args.palette):
        parser.error(f"Palette does not exist: {args.palette}")
    try:
        Editor(args.image, args.palette).run()
    except KeyboardInterrupt:
        print("\nExited.")
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
