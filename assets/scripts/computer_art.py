"""Native 32x24 computer faces shared by game nameplates and design drafts."""
from PIL import Image, ImageDraw

# Palette roles: shell, rim, shadow, screen, eyes, highlight, accent.
COLORS = {
    'arcade': ((1,16,12,157,16,1,16), (15,16,12,16,1,1,157), (1,16,12,18,16,1,157)),
    'cyberpunk': ((147,16,239,18,157,1,227), (227,16,239,18,1,16,227), (244,16,239,18,157,1,227)),
    'foundry': ((244,8,16,245,7,15,7), (12,8,16,18,1,15,7), (8,12,16,18,7,15,7)),
    'synthwave': ((244,16,18,242,157,1,8), (16,244,18,147,1,8,157), (8,16,18,147,157,1,16)),
    'matrix': ((15,16,18,242,8,1,16), (18,16,242,18,1,8,16), (18,8,242,242,16,1,15)),
}


def icon(palette, theme, option):
    shell, rim, shadow, screen, eyes, highlight, accent = (palette[i] for i in COLORS[theme][option])
    image = Image.new('RGBA', (32, 24))
    draw = ImageDraw.Draw(image)
    shapes = (
        ((3,0),(28,0),(31,3),(31,20),(28,23),(3,23),(0,20),(0,3)),
        ((2,0),(27,0),(31,4),(31,21),(29,23),(2,23),(0,21),(0,2)),
        ((1,0),(30,0),(31,1),(31,22),(30,23),(1,23),(0,22),(0,1)),
    )
    draw.polygon(shapes[option], fill=shell, outline=rim)
    draw.line((3,1,27,1), fill=highlight)
    draw.line((30,4,30,20), fill=shadow)
    draw.rectangle((3,3,28,19), fill=screen, outline=rim)
    draw.line((4,4,27,4), fill=shadow)
    if option == 0:
        # Inward brows and a compact, determined mouth.
        draw.line((7,6,12,8), fill=eyes, width=2)
        draw.line((24,6,19,8), fill=eyes, width=2)
        draw.rectangle((10,10,12,12), fill=eyes)
        draw.rectangle((19,10,21,12), fill=eyes)
        draw.line(((12,16),(14,14),(17,14),(19,16)), fill=eyes)
    elif option == 1:
        # One raised eyebrow, one narrowed eye and an asymmetric grin.
        draw.line((7,6,12,6), fill=eyes)
        draw.line((20,8,24,7), fill=eyes)
        draw.rectangle((9,9,11,11), fill=eyes)
        draw.line((20,10,24,10), fill=eyes, width=2)
        draw.line(((11,14),(13,16),(18,16),(21,13)), fill=eyes)
        draw.line((13,14,18,14), fill=highlight)
    else:
        # A machine face: two square eyes and a segmented mouth display.
        draw.rectangle((7,7,12,12), outline=eyes)
        draw.rectangle((19,7,24,12), outline=eyes)
        draw.rectangle((9,9,10,10), fill=highlight)
        draw.rectangle((21,9,22,10), fill=highlight)
        draw.line((13,9,18,9), fill=accent)
        for x in (11,14,17,20):
            draw.line((x,15,x+1,15), fill=eyes)
    # Small hardware details stay outside the face.
    for x in (5,8,11):
        draw.line((x,21,x+1,21), fill=shadow)
    draw.point((27,21), fill=accent)
    if theme == 'arcade' and option == 2:
        for x, index in zip(range(5,25,3),(227,199,157,142,87,8,58)):
            draw.line((x,21,x+1,21), fill=palette[index])
    elif theme == 'foundry':
        for x,y in ((1,2),(30,2),(1,21),(30,21)):
            draw.point((x,y), fill=highlight)
            draw.point((x,y+1), fill=shadow)
        if option == 2:
            # A recessed grille replaces the distracting bright warning ticks.
            draw.rectangle((5,20,24,22), fill=palette[12])
            for x in range(7,24,3):
                draw.line((x,21,x,22), fill=shadow)
            for x in (1,30):
                draw.point((x,21), fill=palette[12])
    elif theme == 'cyberpunk':
        draw.line((1,6,1,11), fill=accent)
        draw.line((30,13,30,18), fill=rim)
    elif theme == 'synthwave':
        for x, color in ((5,rim),(11,accent),(17,eyes)):
            draw.line((x,21,x+4,21), fill=color)
    elif theme == 'matrix':
        draw.line((1,7,1,10), fill=accent)
        draw.point((30,15), fill=highlight)
    visible = {pixel[:3] for pixel in image.getdata() if pixel[3]}
    assert visible <= set(palette) and len(visible) <= 16
    return image
