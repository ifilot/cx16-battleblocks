"""Build a flat sixteen-color Synthwave palette, frames and background."""
import json
from pathlib import Path

from build_foundry import PIECE_ROLES, pack_palette, rgb

ROOT = Path(__file__).resolve().parents[2]
COLORS = (0x000, 0x112, 0x213, 0x324, 0x536, 0xF3D, 0x3CE, 0xFED,
          0xF96, 0x3CD, 0xED4, 0xA6D, 0x5C7, 0xD45, 0x58D, 0xD5A)


def build():
    original = json.loads((ROOT / 'assets/palette/light.json').read_text())
    palette = [min(COLORS, key=lambda c: sum((a - b) ** 2 for a, b in zip(rgb(c), old)))
               for old in original]
    roles = {0: 0x112, 1: 0xFED, 8: 0xF96, 11: 0xF96, 15: 0x536,
             16: 0xF3D, 18: 0x213, 30: 0x213, 46: 0xFED, 62: 0xFED,
             78: 0xFED, 147: 0x213, 232: 0x536, 233: 0x324,
             234: 0x213, 235: 0x213, 236: 0x112, 237: 0x112,
             238: 0x536, 239: 0x000, 240: 0xFED,
             242: 0x112, 243: 0x324, 244: 0x536, 245: 0xF3D,
             246: 0xF3D, 247: 0x3CE, 248: 0x000, 249: 0x000}
    for (edge, highlight, face, shade), color in zip(PIECE_ROLES, COLORS[9:]):
        roles.update({edge: 0x000, highlight: 0xFED, face: color, shade: 0x536})
    for index, color in roles.items():
        palette[index] = color
    assert set(palette) == set(COLORS) and len(COLORS) == 16
    (ROOT / 'src/SYNTHP.DAT').write_bytes(pack_palette(palette))
    (ROOT / 'assets/palette/synthwave.json').write_text(json.dumps([rgb(c) for c in palette]) + '\n')

    # Keep the Commander's transparent font glyphs and all shared game art.
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
    (ROOT / 'src/SYNTH.DAT').write_bytes(tiles)
    print('Synthwave: sixteen colors, neon frames and a flat background')


if __name__ == '__main__':
    build()
