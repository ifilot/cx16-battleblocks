"""Render the approved isometric metropolis directly at 640x480 in 16 colors."""
from pathlib import Path
import random

from PIL import Image, ImageDraw, ImageFilter
from bitmap_font import FONT, SHAPES
from convert_titlescreen import encode_splash

ROOT = Path(__file__).resolve().parents[2]
PIECE_COLORS = ["77FFFF", "0033FF", "7700FF", "00CC33", "FFBB00", "CC0000", "FF00BB"]
BAYER = ((0,8,2,10), (12,4,14,6), (3,11,1,9), (15,7,13,5))

class Canvas:

    def __init__(self, neutrals, base=0):
        self.image = Image.new("P", (640, 480), base)
        palette = neutrals + PIECE_COLORS + ["FFFFFF"]
        values = [int(color[i:i+2], 16) for color in palette for i in (0, 2, 4)]
        self.image.putpalette(values + [0] * (768 - len(values)))
        self.draw = ImageDraw.Draw(self.image)

    def text(self, words, cx, y, scale, color=7, stroke=0, shadow=0):
        x = cx - (len(words) * 6 - 1) * scale // 2
        mask = Image.new("L", (640, 480))
        pen = ImageDraw.Draw(mask)
        for n, letter in enumerate(words):
            for v, row in enumerate(FONT[letter]):
                for u, bit in enumerate(row):
                    if bit == "1":
                        a, b = x + (n * 6 + u) * scale, y + v * scale
                        pen.rectangle((a, b, a + scale - 1, b + scale - 1), fill=255)
        outer = mask.filter(ImageFilter.MaxFilter(stroke * 2 + 1)) if stroke else mask
        if shadow:
            shifted = Image.new("L", (640, 480))
            shifted.paste(outer, (shadow, shadow))
            self.image.paste(0, (0, 0), shifted)
        if stroke:
            self.image.paste(0, (0, 0), outer)
        self.image.paste(color, (0, 0), mask)

    def pattern(self, box, base, ink, coverage=4, step=1):
        self.draw.rectangle(box, fill=base)
        x0, y0, x1, y1 = box
        for y in range(max(y0, 0), min(y1 + 1, 480), step):
            for x in range(max(x0, 0), min(x1 + 1, 640), step):
                if BAYER[(y // step) % 4][(x // step) % 4] < coverage:
                    self.draw.rectangle((x, y, x + step - 1, y + step - 1), fill=ink)

    def footer(self, y=446, color=6):
        self.text("HIT SPACE TO START", 320, y, 2, color, stroke=1)



def project(u, v, z=0):
    return (320 + (u-v)*22, 219 + (u+v)*11 - z)


def scene(c):
    # The floor and every cube share these exact basis vectors and cell size.
    floor = [project(0,0), project(10,0), project(10,10), project(0,10)]
    c.draw.polygon([(x,y+5) for x,y in floor], fill=0)
    c.draw.polygon(floor, fill=2, outline=0)
    for n in range(1,10):
        c.draw.line([project(n,0),project(n,10)], fill=3)
        c.draw.line([project(0,n),project(10,n)], fill=3)
    c.draw.line([project(0,0),project(10,0)], fill=4)
    c.draw.line([project(0,0),project(0,10)], fill=4)
    pieces = [('O',1,1,0), ('T',5,1,0), ('L',7,3,0), ('J',1,4,0),
              ('I',0,7,1), ('S',4,6,0), ('Z',6,8,0)]
    cubes = []
    for kind, u, v, rotation in pieces:
        color, cells = SHAPES[kind]
        for _ in range(rotation):
            cells = [(-y,x) for x,y in cells]
        min_x, min_y = min(x for x,y in cells), min(y for x,y in cells)
        cells = [(u+x-min_x,v+y-min_y) for x,y in cells]
        assert len(set(cells)) == 4
        cubes.extend((x,y,color) for x,y in cells)
    assert len({(u,v) for u,v,color in cubes}) == 28
    assert all(0 <= u < 10 and 0 <= v < 10 for u,v,color in cubes)
    # Cast short contact shadows along the same projected plane.
    for u,v,color in cubes:
        c.draw.polygon([(x+4,y+3) for x,y in
                        (project(u,v),project(u+1,v),project(u+1,v+1),project(u,v+1))], fill=1)
    for u,v,color in sorted(cubes, key=lambda cell:(cell[0]+cell[1],cell[0])):
        a,b,d,e = (project(u,v,12),project(u+1,v,12),
                   project(u+1,v+1,12),project(u,v+1,12))
        floor_b,floor_d,floor_e = project(u+1,v),project(u+1,v+1),project(u,v+1)
        c.draw.polygon([e,d,floor_d,floor_e], fill=1, outline=0)
        c.draw.polygon([b,d,floor_d,floor_b], fill=color, outline=0)
        # Restrained side-face hatching, clipped to its integer parallelogram.
        for x in range(d[0]+1,b[0]):
            top = b[1] + (b[0]-x)//2
            for y in range(top+2,top+11):
                if (x+y)%3 == 0:
                    c.draw.point((x,y), fill=1)
        c.draw.polygon([a,b,d,e], fill=color, outline=0)
        c.draw.line([(e[0]+2,e[1]),(a[0],a[1]+1),(b[0]-2,b[1])], fill=15)


def build():
    active = ROOT / 'assets/png/titlescreen.png'
    c = Canvas(['001122','112233','114455','337777','559999','88BBBB','EEEEFF','FFEECC'])
    c.pattern((0,0,639,479),0,1,5)
    for n,(base,ink) in enumerate([(0,1),(1,2),(2,3),(3,4)]):
        for band in range(4):
            c.pattern((0,128+n*24+band*6,639,133+n*24+band*6),base,ink,band*4)
    rng = random.Random(16)
    for x in range(0,640,32):
        top = rng.choice([160,176,192,208])
        c.draw.rectangle((x,top,x+27,276),fill=1)
        c.draw.line((x,top,x+27,top),fill=3)
        for y in range(top+8,267,12):
            for u in (x+6,x+18):
                c.draw.rectangle((u,y,u+2,y+3),fill=4)
    scene(c)
    c.text('BATTLE',320,22,9,7,stroke=3,shadow=4)
    c.text('BLOCKS',320,91,9,6,stroke=3,shadow=4)
    c.draw.line((100,174,540,174),fill=5)
    c.text('PLAYER VS COMPUTER',320,187,2,15,stroke=2)
    c.footer(449,7)
    assert c.image.size == (640,480) and c.image.mode == 'P'
    assert len(c.image.getcolors()) <= 16
    encode_splash(c.image)
    c.image.save(active)
    print('Native isometric splash: 640x480, 16 colors')



if __name__ == '__main__':
    build()
