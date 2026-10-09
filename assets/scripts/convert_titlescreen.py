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

from pathlib import Path
import sys

from PIL import Image


ROOT = Path(__file__).resolve().parent


def encode_splash(image):
    """Encode native 16x16 tiles, a 64x32 map, and a lossless X16 palette."""
    if image.size != (640, 480):
        raise ValueError("The splash must be 640x480 pixels")

    rgb = image.convert("RGB")
    colors = rgb.getcolors(16)
    if colors is None:
        raise ValueError("The splash must use no more than 16 colors")
    palette = sorted(color for _, color in colors)
    if any(channel % 17 for color in palette for channel in color):
        raise ValueError("Palette channels must be multiples of 17 for X16 RGB")

    indices = {color: index for index, color in enumerate(palette)}
    pixels = rgb.load()
    unique_tiles = {}
    data = bytearray()
    tilemap = bytearray(64 * 32 * 2)
    for tile_y in range(30):
        for tile_x in range(40):
            tile = bytearray()
            for y in range(tile_y * 16, tile_y * 16 + 16):
                for x in range(tile_x * 16, tile_x * 16 + 16, 2):
                    tile.append((indices[pixels[x, y]] << 4) | indices[pixels[x + 1, y]])
            tile = bytes(tile)
            if tile not in unique_tiles:
                unique_tiles[tile] = len(unique_tiles)
                data.extend(tile)
            tile_id = unique_tiles[tile]
            offset = (tile_y * 64 + tile_x) * 2
            tilemap[offset] = tile_id & 255
            tilemap[offset + 1] = tile_id >> 8

    # The tilemap starts at $1E000; its bytes must not overlap the tile set.
    if len(data) > 0x1E000:
        raise ValueError(f"Splash has {len(unique_tiles)} tiles; maximum is 960")

    palette_data = bytearray()
    for red, green, blue in palette:
        palette_data.extend(((green // 17 << 4) | blue // 17, red // 17))
    palette_data.extend(bytes(32 - len(palette_data)))
    return bytes(data), bytes(tilemap), bytes(palette_data)


def main():
    if len(sys.argv) != 5:
        raise SystemExit("Usage: convert_titlescreen.py INPUT.png TILES.DAT MAP.DAT PALETTE.DAT")
    source = ROOT.parent / "png" / sys.argv[1]
    with Image.open(source) as image:
        tiles, tilemap, palette = encode_splash(image)
    # screen_load_tiles uses secondary address 2: raw data, no load header.
    Path(sys.argv[2]).write_bytes(tiles)
    Path(sys.argv[3]).write_bytes(tilemap)
    Path(sys.argv[4]).write_bytes(palette)
    print(f"Native splash: {len(tiles) // 128} tiles, {len(tiles) + len(tilemap)} VRAM bytes, 16 colors")


if __name__ == "__main__":
    main()
