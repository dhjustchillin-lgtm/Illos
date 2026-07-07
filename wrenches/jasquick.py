import argparse
import os
import sys
from PIL import Image

def parse_jasc_pal(pal_path):
    """Parses a JASC-PAL file and returns a list of (R, G, B) tuples."""
    if not os.path.exists(pal_path):
        print(f"Error: Palette file not found at '{pal_path}'", file=sys.stderr)
        sys.exit(1)
        
    with open(pal_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = [line.strip() for line in f.readlines() if line.strip()]
        
    if not lines or lines[0] != "JASC-PAL":
        print("Error: Invalid palette format. Must be a JASC-PAL file.", file=sys.stderr)
        sys.exit(1)
        
    # lines[1] is version (e.g., "0100"), lines[2] is number of colors
    try:
        num_colors = int(lines[2])
    except (IndexError, ValueError):
        print("Error: Could not read the number of colors from palette.", file=sys.stderr)
        sys.exit(1)
        
    colors = []
    for line in lines[3:3 + num_colors]:
        parts = line.split()
        if len(parts) >= 3:
            try:
                colors.append((int(parts[0]), int(parts[1]), int(parts[2])))
            except ValueError:
                continue
                
    if not colors:
        print("Error: No valid colors found in the palette file.", file=sys.stderr)
        sys.exit(1)
        
    return colors

def get_unicode_char(brightness):
    """Maps a 0-255 brightness value to a Unicode block character."""
    # From darkest/emptiest to brightest/fullest
    blocks = [" ", "░", "▒", "▓", "█"]
    index = int(brightness / 256 * len(blocks))
    return blocks[min(index, len(blocks) - 1)]

def render_image(image_path, colors):
    """Renders the image at its original pixel size using TrueColor ANSI escape sequences."""
    if not os.path.exists(image_path):
        print(f"Error: Image file not found at '{image_path}'", file=sys.stderr)
        sys.exit(1)
        
    try:
        with Image.open(image_path) as img:
            # Convert to RGB to ensure standard 3-channel data
            img = img.convert("RGB")
            width, height = img.size
            
            for y in range(height):
                line_chars = []
                for x in range(width):
                    r, g, b = img.getpixel((x, y))
                    
                    # Find the closest color in our JASC palette using Euclidean distance
                    closest_color = min(colors, key=lambda c: (c[0]-r)**2 + (c[1]-g)**2 + (c[2]-b)**2)
                    cr, cg, cb = closest_color
                    
                    # Calculate perceived brightness for the block character selection
                    # Standard luminance formula: 0.299R + 0.587G + 0.114B
                    brightness = int(0.299 * cr + 0.587 * cg + 0.114 * cb)
                    char = get_unicode_char(brightness)
                    
                    # TrueColor (24-bit) ANSI escape code for foreground color: \x1b[38;2;R;G;Bm
                    ansi_color = f"\x1b[38;2;{cr};{cg};{cb}m"
                    line_chars.append(f"{ansi_color}{char}")
                
                # Print the line and reset color at the end of each row (\x1b[0m)
                print("".join(line_chars) + "\x1b[0m")
                
    except Exception as e:
        print(f"Error processing image: {e}", file=sys.stderr)
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(
        description="Render an image in the terminal using Unicode blocks mapped to a JASC-PAL palette."
    )
    parser.add_argument("image_path", help="Path to the source image file.")
    parser.add_argument("pal_path", help="Path to the .pal JASC palette file.")
    
    args = parser.parse_args()
    
    colors = parse_jasc_pal(args.pal_path)
    render_image(args.image_path, colors)

if __name__ == "__main__":
    main()

