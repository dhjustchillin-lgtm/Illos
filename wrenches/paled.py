import sys
import os
import tty
import termios
from PIL import Image

def parse_jasc_pal(file_path):
    """Parses a JASC-PAL file and returns a list of [R, G, B] lists."""
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        sys.exit(1)
        
    colors = []
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = [line.strip() for line in f.readlines() if line.strip()]
        
    if len(lines) < 3 or lines[0] != "JASC-PAL" or lines[1] != "0100":
        print("Error: Not a valid JASC-PAL file.")
        sys.exit(1)
        
    try:
        num_colors = int(lines[2])
        for line in lines[3:3 + num_colors]:
            parts = line.split()
            if len(parts) >= 3:
                r, g, b = map(int, parts[:3])
                colors.append([r, g, b])
    except ValueError:
        print("Error: Failed to parse color data.")
        sys.exit(1)
        
    return colors

def save_jasc_pal(file_path, colors):
    """Saves the current palette back to the JASC-PAL format."""
    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write("JASC-PAL\n")
            f.write("0100\n")
            f.write(f"{len(colors)}\n")
            for r, g, b in colors:
                f.write(f"{r} {g} {b}\n")
        return f"Saved successfully to {os.path.basename(file_path)}!"
    except Exception as e:
        return f"Error saving file: {e}"

def get_char():
    """Reads a single keypress from the terminal without waiting for Enter."""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
        if ch == '\x1b':  # Arrow keys/sequences
            ch += sys.stdin.read(2)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def render_indexed_image_string(img_path, colors):
    """Generates the terminal character block payload for the 64x64 PNG image."""
    if not os.path.exists(img_path):
        return f"[!] Image not found at: {os.path.basename(img_path)}"

    try:
        with Image.open(img_path) as img:
            img = img.convert("P")
            img = img.resize((64, 64))
            
            output = []
            for y in range(64):
                line = ""
                for x in range(64):
                    color_idx = img.getpixel((x, y))
                    if color_idx < len(colors):
                        r, g, b = colors[color_idx]
                    else:
                        r, g, b = 0, 0, 0
                    line += f"\033[48;2;{r};{g};{b}m  "
                output.append(line + "\033[0m")
            return "\n".join(output)
    except Exception as e:
        return f"[!] Error rendering image asset: {e}"

def render_interface(file_path, img_path, colors, selected_index, active_slider, edit_buffer, status_msg, show_image):
    """Clears and renders view state framing: conditional Image + Sliders + Swatch Rows."""
    print("\033[H\033[J", end="")  # Full clear screen and home cursor
    
    # 1. Image View Header Layer
    if show_image and img_path:
        print(render_indexed_image_string(img_path, colors))
        print(f"\n[ Asset Link: {os.path.basename(img_path)} ]")
        print("=" * 90)

    # 2. Palette Settings Table
    print("--- Interactive Palette Editor (Up/Down: Navigate | Left/Right: Sliders | Ctrl+S: Save) ---")
    for index, (r, g, b) in enumerate(colors):
        hex_color = f"#{r:02x}{g:02x}{b:02x}"
        if index == selected_index:
            pointer = " > "
            color_block = f"\033[48;2;{r};{g};{b}m\033[38;2;255;255;255m >       < \033[0m"
            line_style = "\033[1m"
        else:
            pointer = "   "
            color_block = f"\033[48;2;{r};{g};{b}m           \033[0m"
            line_style = ""
        print(f"{pointer}{line_style}[{index:02d}] {color_block}  HEX: {hex_color} | RGB: ({r:3}, {g:3}, {b:3})\033[0m")
    
    print("-" * 90)
    
    # 3. Dynamic Real-time Sliders
    cr, cg, cb = colors[selected_index]
    def draw_slider(label, val, prefix, is_active):
        dots = int((val / 255) * 20)
        bar = "█" * dots + "-" * (20 - dots)
        marker = "> " if is_active else "  "
        return f"{marker}{label}: [{bar}] {val:3}  {prefix}"

    print(draw_slider("R", cr, f"\033[48;2;{cr};0;0m      \033[0m", active_slider == 0))
    print(draw_slider("G", cg, f"\033[48;2;0;{cg};0m      \033[0m", active_slider == 1))
    print(draw_slider("B", cb, f"\033[48;2;0;0;{cb}m      \033[0m", active_slider == 2))
    print("-" * 90)
    
    # 4. Status Bar Context lines
    if edit_buffer:
        print(f"Typing value: {edit_buffer}")
    elif status_msg:
        print(f"[*] {status_msg}")
    else:
        print("Use Left/Right to nudge RGB channels. Press R, G, or B to change track selection.")

def parse_direct_input(buffer_str):
    buffer_str = buffer_str.strip()
    if buffer_str.startswith("#"):
        hex_val = buffer_str.lstrip('#')
        if len(hex_val) != 6:
            raise ValueError
        return [int(hex_val[i:i+2], 16) for i in (0, 2, 4)]
    else:
        parts = buffer_str.replace(',', ' ').split()
        if len(parts) != 3:
            raise ValueError
        rgb = [int(p) for p in parts]
        if any(c < 0 or c > 255 for c in rgb):
            raise ValueError
        return rgb

def main_interactive_loop(file_path, img_path, colors, show_image):
    selected_index = 0
    active_slider = 0  
    status_msg = "Ready."
    edit_buffer = ""
    
    while True:
        render_interface(file_path, img_path, colors, selected_index, active_slider, edit_buffer, status_msg, show_image)
        status_msg = "" 
        
        try:
            key = get_char()
        except KeyboardInterrupt:
            print("\nExiting without saving changes.")
            break
            
        if edit_buffer:
            if key in ('\r', '\n'):
                try:
                    colors[selected_index] = parse_direct_input(edit_buffer)
                    status_msg = f"Updated entry row [{selected_index:02d}]!"
                except ValueError:
                    status_msg = "Invalid Format! Match standard syntax like '#FFFFFF' or '255 255 255'."
                edit_buffer = ""
            elif key in ('\x7f', '\x08'):
                edit_buffer = edit_buffer[:-1]
            elif key == '\x1b':
                edit_buffer = ""
                status_msg = "Entry discarded."
            elif len(key) == 1 and key.isprintable():
                edit_buffer += key
            continue

        if key == '\x1b[A':  # Up Arrow
            selected_index = (selected_index - 1) % len(colors)
        elif key == '\x1b[B':  # Down Arrow
            selected_index = (selected_index + 1) % len(colors)
        elif key == '\x1b[C':  # Right Arrow
            colors[selected_index][active_slider] = min(255, colors[selected_index][active_slider] + 5)
        elif key == '\x1b[D':  # Left Arrow
            colors[selected_index][active_slider] = max(0, colors[selected_index][active_slider] - 5)
        elif key.lower() == 'r':
            active_slider = 0
        elif key.lower() == 'g':
            active_slider = 1
        elif key.lower() == 'b':
            active_slider = 2
        elif key == '#':
            edit_buffer = "#"
        elif key.isdigit():
            edit_buffer = key
        elif key == '\x13':  # Ctrl+S
            status_msg = save_jasc_pal(file_path, colors)
        elif key == '\x03':  # Ctrl+C
            print("\nExited Live Workspace.")
            break

if __name__ == "__main__":
    args = sys.argv[1:]
    if not args or "-h" in args or "--help" in args:
        print("Usage: python palview_term.py <path_to_palette.pal> [-i] [--edit]")
        sys.exit(1)
        
    pal_path = args[0]
    show_image = "-i" in args
    edit_mode = "--edit" in args
    
    palette_colors = parse_jasc_pal(pal_path)
    img_path = os.path.join(os.path.dirname(os.path.abspath(pal_path)), "front.png") if show_image else None
    
    if edit_mode:
        main_interactive_loop(pal_path, img_path, palette_colors, show_image)
    else:
        # Simple read-only print fallback
        if show_image:
            print(render_indexed_image_string(img_path, palette_colors))
        print("\n--- Palette Colors ---")
        for index, (r, g, b) in enumerate(palette_colors):
            hex_color = f"#{r:02x}{g:02x}{b:02x}"
            color_block = f"\033[48;2;{r};{g};{b}m      \033[0m"
            print(f"[{index:02d}] {color_block}  HEX: {hex_color} | RGB: ({r:3}, {g:3}, {b:3})")
        print("----------------------\n")

