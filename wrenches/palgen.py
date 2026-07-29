import sys
from pathlib import Path
from PIL import Image

def png_to_jasc_pal(input_path):
    png_path = Path(input_path)
    
    # Create output path in the same directory with .pal extension
    pal_path = png_path.with_suffix('.pal')
    
    # Open image and convert to 256-color adaptive palette
    img = Image.open(png_path).convert('P', palette=Image.ADAPTIVE, colors=256)
    
    # Extract RGB values
    palette = img.getpalette()
    colors = [palette[i:i+3] for i in range(0, len(palette), 3)]
    
    # Write to JASC-PAL file format
    with open(pal_path, 'w') as f:
        f.write("JASC-PAL\n")
        f.write("0100\n")
        f.write(f"{len(colors)}\n")
        for r, g, b in colors:
            f.write(f"{r} {g} {b}\n")
            
    print(f"Saved: {pal_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python convert.py <path_to_png> [additional_pngs...]")
        sys.exit(1)
        
    # Process all paths passed in as arguments
    for path_arg in sys.argv[1:]:
        png_to_jasc_pal(path_arg)

