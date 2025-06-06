from PIL import Image

def quantize_color(color):
    """Keep only the upper 4 bits of each RGBA channel."""
    return tuple((channel & 0xF0) for channel in color)

def extract_colors(image_path):
    img = Image.open(image_path).convert("RGBA")
    colors = []
    
    cell_width, cell_height = 16, 10
    for row in range(16):
        for col in range(16):
            # Top-left corner of the cell
            x = col * cell_width
            y = row * cell_height
            # Sample the top-left pixel of the cell
            print((x+8, y+5))
            pixel = img.getpixel((x+8, y))
            colors.append(pixel)
    return colors

def build_palette_image(colors, cell_width=64, cell_height=64):
    """Create a new image from the modified colors."""
    out_img = Image.new("RGBA", (16 * cell_width, 16 * cell_height))
    for i, color in enumerate(colors):
        col = i % 16
        row = i // 16
        for y in range(row * cell_height, (row + 1) * cell_height):
            for x in range(col * cell_width, (col + 1) * cell_width):
                out_img.putpixel((x, y), color)
    return out_img

def process_palette(image_path, output_path="quantized_palette.png"):
    original_colors = extract_colors(image_path)
    reduced_colors = [quantize_color(c) for c in original_colors]
    new_img = build_palette_image(reduced_colors)
    new_img.save("../palette/palette-vga.png")

# Example usage
process_palette("../palette/palette-base.png")
