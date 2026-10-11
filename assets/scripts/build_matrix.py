"""Build sixteen-color Matrix surfaces with sparse terminal code wallpaper."""
import json
from pathlib import Path

from build_foundry import PIECE_ROLES, pack_palette, rgb

ROOT = Path(__file__).resolve().parents[2]
COLORS = (0x000, 0x011, 0x021, 0x032, 0x063, 0x4E7, 0x8FA, 0xCFE,
          0x294, 0x3CD, 0xED4, 0xA6D, 0x5C7, 0xD45, 0x58D, 0xD5A)


def build():
    original = json.loads((ROOT / 'assets/palette/light.json').read_text())
    palette = [min(COLORS, key=lambda c: sum((a - b) ** 2 for a, b in zip(rgb(c), old)))
               for old in original]
    roles = {0: 0x011, 1: 0xCFE, 8: 0x8FA, 11: 0x8FA, 15: 0x063,
             16: 0x4E7, 18: 0x021, 30: 0x021, 46: 0xCFE, 62: 0xCFE,
             78: 0xCFE, 147: 0x021, 232: 0x063, 233: 0x032,
             234: 0x021, 235: 0x021, 236: 0x011, 237: 0x011,
             238: 0x063, 239: 0x000, 240: 0xCFE,
             242: 0x011, 243: 0x032, 244: 0x063, 245: 0x4E7,
             246: 0x4E7, 247: 0x8FA, 248: 0x000, 249: 0x000}
    for (edge, highlight, face, shade), color in zip(PIECE_ROLES, COLORS[9:]):
        roles.update({edge: 0x000, highlight: 0xCFE, face: color, shade: 0x063})
    for index, color in roles.items():
        palette[index] = color
    assert set(palette) == set(COLORS) and len(COLORS) == 16
    (ROOT / 'src/MATRIXP.DAT').write_bytes(pack_palette(palette))
    (ROOT / 'assets/palette/matrix.json').write_text(json.dumps([rgb(c) for c in palette]) + '\n')

    # Keep transparent font glyphs and all shared game art.
    tiles = bytearray((ROOT / 'src/ARCADE.DAT').read_bytes())
    origins = {5: (-16, -16), 6: (48, -16), 7: (-16, 48), 8: (48, 48),
               9: (16, -16), 10: (48, 16), 11: (-16, 16), 12: (16, 48)}
    for tile, (left, top) in origins.items():
        for y in range(16):
            for x in range(16):
                px, py = left + x, top + y
                color = 239 if 0 <= px < 53 and 0 <= py < 53 else 0
                if -3 <= px < 51 and -3 <= py < 51:
                    color = 16
                    if px == -3 or py == -3:
                        color = 8
                if 0 <= px < 48 and 0 <= py < 48:
                    color = 0
                tiles[tile * 256 + y * 16 + x] = color
    (ROOT / 'src/MATRIX.DAT').write_bytes(tiles)
    # Sparse static code columns decorate only exposed wallpaper cells.
    wall = bytearray([236] * (16 * 256))
    for ty in range(4):
        for row in range(2):
            bits = (0b111, 0b101, 0b101, 0b101, 0b111) if (ty + row) % 2 else (0b010, 0b110, 0b010, 0b010, 0b111)
            for y, mask in enumerate(bits):
                for x in range(3):
                    if mask & (4 >> x):
                        wall[(ty * 4) * 256 + (row * 8 + y + 1) * 16 + x + 6] = 243
    (ROOT / 'src/MATBG.DAT').write_bytes(wall)
    print('Matrix: sixteen colors, phosphor frames and sparse code columns')


if __name__ == '__main__':
    build()
