"""Native split-card drawing and miniatures used by the HUD builder."""
from pathlib import Path
from PIL import Image, ImageDraw
from bitmap_font import FONT
from convert_tilemap import get_color_palette, build_statistics_tiles

ROOT = Path(__file__).resolve().parents[2]
WHITE=(255,255,255); PAPER=(238,238,238); INK=(51,51,51); TEAL=(102,136,136)

class Canvas:
    def __init__(self, base):
        self.image = base.copy().convert("RGB")
        self.draw = ImageDraw.Draw(self.image)

    def text(self,s,x,y,color=INK,scale=1,right=False):
        font=FONT
        width=6*scale
        if right: x-=len(s)*width-scale
        for i,char in enumerate(s):
            for row,bits in enumerate(font[char]):
                for col,bit in enumerate(bits):
                    if bit=='1':
                        a=x+i*width+col*scale;b=y+row*scale
                        self.draw.rectangle((a,b,a+scale-1,b+scale-1),fill=color)

    def panel(self,x,y,w,h,fill,edge=INK,shadow=True,bevel=False):
        if shadow: self.draw.rectangle((x+2,y+2,x+w+1,y+h+1),fill=INK)
        self.draw.rectangle((x,y,x+w-1,y+h-1),fill=fill,outline=edge)
        if bevel:
            self.draw.line((x+1,y+1,x+w-2,y+1),fill=WHITE)
            self.draw.line((x+1,y+1,x+1,y+h-2),fill=WHITE)

    def icon(self,piece,x,y):
        self.image.paste(ICONS[piece],(x,y),ICONS[piece])

def load_icons():
    data=bytearray((ROOT/'src/TILES.DAT').read_bytes());palette=get_color_palette()
    if len(data) < 144*256: build_statistics_tiles(data)
    icons=[]
    for piece in range(7):
        icon=Image.new('RGBA',(32,16))
        for half in range(2):
            pixels=data[(130+piece*2+half)*256:(131+piece*2+half)*256]
            for y in range(16):
                for x in range(16):
                    value=pixels[y*16+x]
                    if value: icon.putpixel((half*16+x,y),tuple(int(v) for v in palette[value])+ (255,))
        icons.append(icon)
    return icons
