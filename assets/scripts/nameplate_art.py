"""Approved native player/controller and computer/monitor banner artwork."""
from pathlib import Path
from PIL import Image, ImageDraw
from bitmap_font import FONT

ROOT = Path(__file__).resolve().parents[2]
PALETTE_DIR = ROOT / 'assets/palette'


def text(image, value, x, y, color, scale=1):
    draw = ImageDraw.Draw(image)
    for i, character in enumerate(value):
        for row, bits in enumerate(FONT[character]):
            for col, bit in enumerate(bits):
                if bit == '1':
                    left = x + (i * 6 + col) * scale
                    top = y + row * scale
                    draw.rectangle((left, top, left + scale - 1, top + scale - 1), fill=color)


def controller(palette, cyber):
    image = Image.new('RGBA', (32, 24))
    d = ImageDraw.Draw(image)
    ink = palette[11]
    shell = palette[147 if cyber else 15]
    dark = palette[242 if cyber else 12]
    highlight = palette[1]
    # Cable and the familiar rounded European SNES silhouette.
    d.line(((15, 0), (15, 2), (17, 4), (17, 6)), fill=ink)
    shape = [(5, 6), (26, 6), (29, 8), (31, 12), (31, 18),
             (28, 21), (23, 21), (20, 18), (11, 18), (8, 21),
             (3, 21), (0, 18), (0, 12), (2, 8)]
    d.polygon(shape, fill=shell)
    d.line(shape + [shape[0]], fill=ink)
    d.line((5, 7, 26, 7), fill=highlight)
    d.line((2, 17, 4, 19), fill=dark)
    d.line((24, 19, 27, 19), fill=dark)
    # D-pad and small slanted Select/Start buttons.
    pad = palette[11 if not cyber else 1]
    d.rectangle((6, 10, 8, 16), fill=pad)
    d.rectangle((4, 12, 10, 14), fill=pad)
    d.point((7, 11), fill=dark)
    d.line((13, 14, 15, 12), fill=pad)
    d.line((17, 14, 19, 12), fill=pad)
    # X blue, Y green, A red, B yellow: European four-color buttons.
    for x, y, index in ((25, 9, 199), (22, 12, 142),
                         (28, 12, 58 if cyber else 59), (25, 15, 87)):
        d.rectangle((x - 1, y - 1, x + 1, y + 1), fill=palette[index])
        d.point((x - 1, y - 1), fill=highlight)
    return image


def nameplate(image, palette, cyber, offset, icon, label, role, foundry=False, synthwave=False):
    d = ImageDraw.Draw(image)
    x, y, w, h = 16 + offset, 0, 272, 26
    # A light card or dark neon card, following the existing score-card style.
    d.rectangle((x + 2, y + 2, x + w + 1, y + h + 1), fill=palette[239])
    d.rectangle((x, y, x + w - 1, y + h - 1), fill=palette[30], outline=palette[8 if foundry else 16 if synthwave else 11])
    if foundry:
        d.line((x + 1, 1, x + w - 2, 1), fill=palette[7])
        for bolt_x in (x + 2, x + w - 3):
            for bolt_y in (3, 22):
                d.point((bolt_x, bolt_y), fill=palette[15])
    elif not cyber:
        d.line((x + 1, 1, x + w - 2, 1), fill=palette[1])
    d.line((x + 43, 5, x + 43, 20), fill=palette[147])
    image.paste(icon, (x + 6, 1), icon)
    text(image, label, x + 52, 7, palette[247 if foundry else 1 if synthwave else 11], 2)
    text(image, role, x + w - 10 - (len(role) * 6 - 1), 10, palette[11])

