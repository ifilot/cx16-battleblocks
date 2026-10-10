#########################################################################
#                                                                       #
#   Author: Ivo Filot <ivo@ivofilot.nl>                                 #
#                                                                       #
#   CX16-BATTLEBLOCKS is free software:                                 #
#   you can redistribute it and/or modify it under the terms of the     #
#   GNU General Public License as published by the Free Software        #
#   Foundation, either version 3 of the License, or (at your option)    #
#   any later version.                                                  #
#                                                                       #
#   CX16-BATTLEBLOCKS is distributed in the hope that it will be useful,#
#   but WITHOUT ANY WARRANTY; without even the implied warranty         #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.             #
#   See the GNU General Public License for more details.                #
#                                                                       #
#   You should have received a copy of the GNU General Public License   #
#   along with this program.  If not, see http://www.gnu.org/licenses/. #
#                                                                       #
#########################################################################

import PIL.Image
import os
import numpy as np
import sys

ROOT = os.path.dirname(__file__)

def main():
    """
    Produce a TILEMAP for the CX16 by opening a .png file containing the tiles
    and a .png file containing the palette. Search for the best color in the
    palette matching any pixel and store the tiles in a color-encoded data file
    based on the palette.
    """
    # grab the colors
    palette = get_color_palette()
    
    # encode tiles based on the color palette
    img = PIL.Image.open(os.path.join(ROOT, '..', 'png', sys.argv[1]))
    data = build_tiles(img, palette)
    if sys.argv[1] == 'playscreen.png':
        build_panel_frames(data)
        # Separate surface colors from text and cards for runtime themes.
        for tile, aliases in ((1, {147: 236, 148: 237}),
                              (2, {26: 232, 28: 233, 29: 234, 30: 235}),
                              (3, {26: 232, 28: 233, 29: 234, 30: 235})):
            for pixel in range(tile * 256, (tile + 1) * 256):
                data[pixel] = aliases.get(data[pixel], data[pixel])
        # The original large font shares white with the card paper.
        for tile in (*range(24, 34), *range(96, 122)):
            for pixel in range(tile * 256, (tile + 1) * 256):
                if data[pixel] == 30:
                    data[pixel] = 240
        # D had a transparent right stroke, visible only on a light grid.
        # Paint it explicitly so the glyph also works on dark surfaces.
        for y in range(4, 10):
            for x in (11, 12):
                data[99 * 256 + y * 16 + x] = 240
        build_menu_punctuation(data)
    
    with open(sys.argv[2], 'wb') as f:
        f.write(data)

def build_menu_punctuation(data):
    """Use five unused base tiles for version numbers, URLs and volume labels."""
    glyphs = (
        ('00000', '00110', '00110', '00000', '00110', '00110', '00000'),
        ('11001', '11010', '00010', '00100', '01000', '01011', '10011'),
    )
    for tile, rows in enumerate(glyphs, 39):
        pixels = bytearray(256)
        points = [(3 + x * 2 + dx, 1 + y * 2 + dy)
                  for y, row in enumerate(rows) for x, bit in enumerate(row) if bit == '1'
                  for dx in range(2) for dy in range(2)]
        for x, y in points:
            pixels[(y + 1) * 16 + x + 1] = 16
        for x, y in points:
            pixels[y * 16 + x] = 240
        data[tile * 256:(tile + 1) * 256] = pixels

    # Match the menu font's two-pixel dark outline and y=2..11 cap height.
    # A baseline dot, centered dash and connected slash stay readable on
    # both the light card surface and the dark Cyberpunk background.
    shapes = (
        {(x, y) for y in (10, 11) for x in range(6, 10)},
        {(10 - (y - 2) // 2 * 2 + dx, y)
         for y in range(2, 12) for dx in range(3)},
        {(x, y) for y in (6, 7) for x in range(3, 13)},
    )
    for tile, points in enumerate(shapes, 36):
        pixels = bytearray(256)
        for x, y in points:
            for dy in range(-2, 3):
                for dx in range(-2, 3):
                    px, py = x + dx, y + dy
                    if 0 <= px < 16 and 0 <= py < 16:
                        pixels[py * 16 + px] = 18
        for x, y in points:
            pixels[y * 16 + x] = 240
        data[tile * 256:(tile + 1) * 256] = pixels


def build_panel_frames(data):
    """Replace the eight striped frame tiles with a rim and offset shadow.

    Draw directly in native 16x16 palette indices. The middle edge samples
    repeat seamlessly; transparent margins reveal the patterned background.
    Tile IDs and panel dimensions stay compatible with the game's tile maps.
    """
    origins = {
        5: (-16, -16), 6: (48, -16),
        7: (-16, 48), 8: (48, 48),
        9: (16, -16), 10: (48, 16),
        11: (-16, 16), 12: (16, 48),
    }
    for tile, (left, top) in origins.items():
        for y in range(16):
            for x in range(16):
                px, py = left + x, top + y
                color = 0
                # Two-pixel drop shadow, tucked against the rim.
                if 0 <= px < 52 and 0 <= py < 52:
                    color = 239
                # Two-pixel rim immediately outside the panel's contents.
                if -2 <= px < 50 and -2 <= py < 50:
                    color = 16
                if 0 <= px < 48 and 0 <= py < 48:
                    color = 0
                data[tile * 256 + y * 16 + x] = color


def build_statistics_tiles(data):
    """Native 8x8 blocks and 7x12 digits, packed into 16x16 game tiles."""
    shapes = (
        ((0, 0), (1, 0), (2, 0), (3, 0)),
        ((0, 0), (1, 0), (0, 1), (1, 1)),
        ((1, 0), (0, 1), (1, 1), (2, 1)),
        ((1, 0), (2, 0), (0, 1), (1, 1)),
        ((0, 0), (1, 0), (1, 1), (2, 1)),
        ((0, 0), (0, 1), (1, 1), (2, 1)),
        ((2, 0), (0, 1), (1, 1), (2, 1)),
    )
    data.extend(bytes(254 * 256 - len(data)))
    for piece, shape in enumerate(shapes):
        source = data[(16 + piece) * 256:(17 + piece) * 256]
        left = (32 - (max(x for x, _ in shape) + 1) * 8) // 2
        top = (16 - (max(y for _, y in shape) + 1) * 8) // 2
        for bx, by in shape:
            for y in range(8):
                for x in range(8):
                    if x in (0, 7) and y in (0, 7):
                        color = 0
                    elif x in (0, 7) or y in (0, 7):
                        color = source[8 * 16]
                    elif x == 1 or y == 1:
                        color = source[2 * 16 + 2]
                    elif x == 6 or y == 6:
                        color = source[13 * 16 + 13]
                    else:
                        color = source[8 * 16 + 8]
                    px, py = left + bx * 8 + x, top + by * 8 + y
                    tile = 130 + piece * 2 + px // 16
                    data[tile * 256 + py * 16 + px % 16] = color
    digits = (
        (14, 17, 19, 21, 25, 17, 14), (4, 12, 4, 4, 4, 4, 14),
        (14, 17, 1, 2, 4, 8, 31), (30, 1, 1, 14, 1, 1, 30),
        (2, 6, 10, 18, 31, 2, 2), (31, 16, 16, 30, 1, 1, 30),
        (14, 16, 16, 30, 17, 17, 14), (31, 1, 2, 4, 8, 8, 8),
        (14, 17, 17, 14, 17, 17, 14), (14, 17, 17, 15, 1, 1, 14),
    )
    for tile in range(144, 254):
        pair = tile - 144
        values = (pair // 10, pair % 10) if tile < 244 else (None, tile - 244)
        # A dark backing keeps the small digits clear over the patterned floor.
        for half, value in enumerate(values):
            if value is None:
                continue
            for y in range(16):
                for x in range(8):
                    on = 2 <= y < 14 and x < 7 and digits[value][(y - 2) * 7 // 12] & (1 << (4 - x * 5 // 7))
                    data[tile * 256 + y * 16 + half * 8 + x] = 1 if on else 16


def build_tiles(img, palette):
    """
    Grab the tiles from an image file and compare the colors with a palette.
    Pick the best matching color and store the tilemap using the color-encoding
    as supplied by the palette.
    """
    data = bytearray()

    rows = img.size[1] // 16
    columns = img.size[0] // 16

    print(rows, columns)
    
    for i in range(rows):      # loop over rows
        for j in range(columns):  # loop over columns
            
            # loop over pixels
            for y in range(16):
                for x in range(16):
                    col = img.getpixel(((j*16+x),(i*16+y)))
                    idx = find_closest_color(col, palette)
                    data.extend(idx)
    return data

def find_closest_color(col, palette):
    """
    Find closest color match from palette given a pixel color
    """
    # if transparent, return 0
    if col[3] < 50:
        return np.uint8(0x00)
    
    distances = np.linalg.norm(palette[1:,:] - col[0:3], axis=1)
    return np.uint8(np.argmin(distances)+1)

def get_color_palette():
    """
    Build the color palette from the CX16 palette png file
    """
    file = os.path.join(ROOT, '..', 'palette', 'cx16palette.png')
    img = PIL.Image.open(file)
    
    colors = []
    for i in range(16):
        for j in range(16):
            col = img.getpixel((j*64+32,i*64+32))
            colors.append(col)
    
    return np.array(colors, np.uint8)
                    
if __name__ == '__main__':
    main()
