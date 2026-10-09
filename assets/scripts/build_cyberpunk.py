"""Pack the approved Microchip circuit wallpaper without resampling.

Sixteen 16x16 tiles form a seamless 64x64 repeat. Palette slots 242-245
are dedicated to this theme. Tiles 368-383 end immediately before MAPBASE0.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]

def build():
    image = Image.open(ROOT / 'assets/png/cyberpunk-background.png')
    assert image.mode == 'P' and image.size == (64, 64)
    pixels = bytes(image.getdata())
    assert set(pixels) <= {242, 243, 244, 245}
    tiles = b"".join(pixels[(ty * 16 + y) * 64 + tx * 16:
                          (ty * 16 + y) * 64 + tx * 16 + 16]
                     for ty in range(4) for tx in range(4) for y in range(16))
    (ROOT / 'src/CYBER.DAT').write_bytes(tiles)

if __name__ == '__main__':
    build()
