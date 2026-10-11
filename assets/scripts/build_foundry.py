"""Build native Foundry steel tiles and three scene palettes.

Foundry uses exactly sixteen distinct 12-bit colors across the whole screen.
Legacy 8bpp pixel indices alias those colors, including HUDs and banners;
no extra colors are introduced by the loading label or menu lettering.
"""
import json
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
COLORS = (0x111, 0x222, 0x333, 0x444, 0xFED, 0xC84, 0xEA6, 0x753,
          0x3CD, 0xED4, 0xA6D, 0x5C7, 0xD45, 0x58D, 0xD5A, 0xCBA)

ARCADE_COLORS = (0x000, 0xEEE, 0xFFF, 0xCCC, 0x223, 0x889, 0x347, 0x46B,
                 0x3CE, 0xED4, 0x96C, 0x5B7, 0xD45, 0x58C, 0xD5A, 0xF94)
PIECE_ROLES = ((167, 150, 157, 156), (84, 73, 87, 86), (223, 220, 227, 225),
               (139, 143, 142, 141), (55, 59, 58, 57), (195, 185, 199, 197),
               (253, 241, 255, 254))


def rgb(color):
    return tuple(((color >> shift) & 15) * 17 for shift in (8, 4, 0))


def pack_palette(colors):
    return bytes(part for color in colors for part in (color & 255, color >> 8))


def pack_tiles(image):
    pixels = image.tobytes()
    width, height = image.size
    return b''.join(pixels[(ty * 16 + y) * width + tx * 16:
                          (ty * 16 + y) * width + tx * 16 + 16]
                    for ty in range(height // 16) for tx in range(width // 16)
                    for y in range(16))


def build():
    palettes = []
    for style, filename in (('light', 'ARCADEP.DAT'), ('cyberpunk', 'CYBERP.DAT')):
        values = json.loads((ROOT / 'assets/palette' / (style + '.json')).read_text())
        palette = [(r // 17 << 8) | (g // 17 << 4) | b // 17 for r, g, b in values]
        # Preserve the resident loading label's two dedicated colors.
        palette[46], palette[62] = 0xFFF, 0x0EF
        palette[246:250] = [0x9EF, 0xBBF, 0x012, 0x001] if style == 'cyberpunk' else [0xFEC, 0xEEF, 0x012, 0x001]
        if style == 'cyberpunk':
            (ROOT / 'src' / filename).write_bytes(pack_palette(palette))
        palettes.append(palette)
    # Neutral case-gray panels and PCB-blue framing follow the X16 hardware.
    arcade = [min(ARCADE_COLORS, key=lambda c: sum((a - b) ** 2 for a, b in zip(rgb(c), rgb(old))))
              for old in palettes[0]]
    arcade_roles = {0: 0x000, 1: 0xFFF, 8: 0xF94, 11: 0x347, 15: 0xCCC, 16: 0x347,
                    18: 0xCCC, 30: 0xFFF, 46: 0xFFF, 62: 0xFFF, 147: 0x347,
                    232: 0xCCC, 233: 0xEEE, 234: 0xFFF, 235: 0xFFF,
                    236: 0xEEE, 237: 0xCCC, 238: 0x889, 239: 0x889, 240: 0x347,
                    242: 0xEEE, 243: 0xCCC, 244: 0xFFF, 245: 0x347,
                    246: 0x46B, 247: 0x3CE, 248: 0x223, 249: 0x889}
    for (edge, highlight, face, shade), color in zip(PIECE_ROLES, ARCADE_COLORS[8:15]):
        arcade_roles.update({edge: 0x000, highlight: 0xFFF, face: color, shade: 0x889})
    for index, color in arcade_roles.items():
        arcade[index] = color
    assert set(arcade) == set(ARCADE_COLORS)
    (ROOT / 'src/ARCADEP.DAT').write_bytes(pack_palette(arcade))
    (ROOT / 'assets/palette/arcade.json').write_text(json.dumps([rgb(c) for c in arcade]) + '\n')
    # Quantize every legacy palette entry, then assign the semantic roles.
    palette = [min(COLORS, key=lambda c: sum((a - b) ** 2 for a, b in zip(rgb(c), rgb(old))))
               for old in palettes[0]]
    roles = {0: 0x222, 1: 0xFED, 7: 0xEA6, 8: 0xC84, 9: 0x753,
             11: 0xEA6, 15: 0xCBA, 16: 0x111, 18: 0x222, 30: 0x222,
             46: 0xEA6, 62: 0xEA6, 147: 0x753,
             232: 0x333, 233: 0x222, 234: 0x222, 235: 0x222,
             236: 0x222, 237: 0x333, 238: 0x444, 239: 0x111, 240: 0xFED,
             242: 0x222, 243: 0x333, 244: 0x444, 245: 0x111,
             246: 0xC84, 247: 0xFED, 248: 0x111, 249: 0x111}
    # Shared ivory highlights and steel shadows leave one face color per piece.
    for (edge, highlight, face, shade), color in zip(PIECE_ROLES, COLORS[8:15]):
        roles.update({edge: 0x111, highlight: 0xFED, face: color, shade: 0x444})
    for index, color in roles.items():
        palette[index] = color
    assert set(palette) == set(COLORS) and len(COLORS) == 16
    (ROOT / 'src/FOUNDRYP.DAT').write_bytes(pack_palette(palette))
    (ROOT / 'assets/palette/foundry.json').write_text(json.dumps([rgb(c) for c in palette]) + '\n')

    # A seamless 64x64 sheet-metal repeat occupies the circuit atlas's slots.
    wallpaper = Image.new('P', (64, 64), 242)
    draw = ImageDraw.Draw(wallpaper)
    for y in range(64):
        for x in range(64):
            if (x * 7 + y * 11) % 41 == 0:
                draw.point((x, y), fill=243)
    draw.line((0, 0, 63, 0), fill=245)
    draw.line((0, 0, 0, 63), fill=245)
    draw.line((1, 1, 63, 1), fill=244)
    draw.line((1, 1, 1, 63), fill=244)
    for x, y in ((6, 6), (57, 6), (6, 57), (57, 57)):
        draw.rectangle((x - 1, y - 1, x + 1, y + 1), fill=245)
        draw.point((x - 1, y - 1), fill=244)
        draw.point((x, y), fill=243)
    (ROOT / 'src/FORGE.DAT').write_bytes(pack_tiles(wallpaper))
    wallpaper.putpalette([part for color in palette for part in rgb(color)])
    wallpaper.save(ROOT / 'assets/png/foundry-background.png')

    # Overlay only the thirteen surface/frame tiles; leave game art resident.
    tiles = bytearray((ROOT / 'src/TILES.DAT').read_bytes()[:13 * 256])
    for tile in (2, 3):
        for y in range(16):
            for x in range(16):
                tiles[tile * 256 + y * 16 + x] = 232 if y == 0 or x == 15 else 235
    origins = {5: (-16, -16), 6: (48, -16), 7: (-16, 48), 8: (48, 48),
               9: (16, -16), 10: (48, 16), 11: (-16, 16), 12: (16, 48)}
    bolts = ((-2, -2), (49, -2), (-2, 49), (49, 49))
    for tile, (left, top) in origins.items():
        for y in range(16):
            for x in range(16):
                px, py = left + x, top + y
                color = 239 if 0 <= px < 54 and 0 <= py < 54 else 0
                if -4 <= px < 52 and -4 <= py < 52:
                    color = 8
                    if px == -4 or py == -4:
                        color = 7
                    elif px == 51 or py == 51:
                        color = 9
                    elif (px * 7 + py * 11) % 37 == 0:
                        color = 9
                if 0 <= px < 48 and 0 <= py < 48:
                    color = 0
                for bx, by in bolts:
                    if abs(px - bx) <= 1 and abs(py - by) <= 1:
                        color = 16 if (px == bx and py == by) else 15
                tiles[tile * 256 + y * 16 + x] = color
    (ROOT / 'src/FOUNDRY.DAT').write_bytes(tiles)
    # Arcade uses a quieter case-gray floor and crisp blue bevels.
    tiles = bytearray((ROOT / 'src/TILES.DAT').read_bytes())
    # Remove the old opaque font backing; retain the established glyph shapes.
    for tile in (*range(24, 34), *range(35, 41), *range(96, 122)):
        for pixel in range(tile * 256, (tile + 1) * 256):
            if tiles[pixel] != 240:
                tiles[pixel] = 0
    tiles[256:512] = bytes([236]) * 256
    for tile, (left, top) in origins.items():
        for y in range(16):
            for x in range(16):
                px, py = left + x, top + y
                color = 239 if 0 <= px < 53 and 0 <= py < 53 else 0
                if -3 <= px < 51 and -3 <= py < 51:
                    color = 16
                    if px == -3 or py == -3:
                        color = 246
                if 0 <= px < 48 and 0 <= py < 48:
                    color = 0
                tiles[tile * 256 + y * 16 + x] = color
    (ROOT / 'src/ARCADE.DAT').write_bytes(tiles)
    print('Arcade and Foundry: sixteen-color palettes and native surface tiles')



if __name__ == '__main__':
    build()
