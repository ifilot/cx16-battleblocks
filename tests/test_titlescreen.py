"""Verify that the splash fits VERA and encodes without changing any pixels."""

import importlib.util
from pathlib import Path
import unittest

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "convert_titlescreen", ROOT / "assets/scripts/convert_titlescreen.py"
)
converter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(converter)


class TitleScreenTests(unittest.TestCase):
    def test_native_artwork_round_trip(self):
        with Image.open(ROOT / "assets/png/titlescreen.png") as image:
            source = image.convert("RGB")
            tiles, tilemap, packed_palette = converter.encode_splash(image)
        self.assertEqual(source.size, (640, 480))
        self.assertEqual(len(source.getcolors(17)), 16)
        self.assertLessEqual(len(tiles), 0x1E000)
        self.assertEqual(len(tilemap), 4096)
        self.assertEqual(len(packed_palette), 32)
        palette = []
        for i in range(0, 32, 2):
            gb, red = packed_palette[i:i + 2]
            palette.append((red * 17, (gb >> 4) * 17, (gb & 15) * 17))
        decoded = bytearray()
        for y in range(480):
            for x in range(640):
                offset = ((y // 16) * 64 + x // 16) * 2
                tile_id = int.from_bytes(tilemap[offset:offset + 2], "little")
                self.assertLess(tile_id, 960)
                pixel_pair = tiles[tile_id * 128 + (y % 16) * 8 + (x % 16) // 2]
                index = (pixel_pair >> 4) if x % 2 == 0 else (pixel_pair & 15)
                decoded.extend(palette[index])
        self.assertEqual(bytes(decoded), source.tobytes())

    def test_repeated_tiles_share_vram(self):
        tiles, tilemap, palette = converter.encode_splash(Image.new("RGB", (640, 480), "#112233"))
        self.assertEqual(len(tiles), 128)
        self.assertEqual(tilemap, bytes(4096))
        self.assertEqual(palette[:2], bytes([0x23, 0x01]))

    def test_rejects_resizing(self):
        with self.assertRaisesRegex(ValueError, "640x480"):
            converter.encode_splash(Image.new("RGB", (320, 240)))

    def test_rejects_color_reduction(self):
        image = Image.new("RGB", (640, 480))
        for x in range(17):
            image.putpixel((x, 0), (x * 17 % 256, 0, 0))
        with self.assertRaisesRegex(ValueError, "16 colors"):
            converter.encode_splash(image)
        with self.assertRaisesRegex(ValueError, "multiples of 17"):
            converter.encode_splash(Image.new("RGB", (640, 480), "#123456"))

    def test_rejects_vram_overflow(self):
        image = Image.new("RGB", (640, 480))
        for tile in range(1200):
            x, y = (tile % 40) * 16, (tile // 40) * 16
            for n in range(3):
                level = ((tile >> (n * 4)) & 15) * 17
                image.putpixel((x + n, y), (level, level, level))
        with self.assertRaisesRegex(ValueError, "maximum is 960"):
            converter.encode_splash(image)


if __name__ == "__main__":
    unittest.main()
