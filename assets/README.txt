Battle Blocks assets

png/playscreen.png: source tile sheet for the game and menu, including
large lettering and next-piece previews.
png/titlescreen.png: approved 640x480, 16-color isometric metropolis splash.
It is reproduced by scripts/render_splash.py using integer pixel geometry.
png/cyberpunk-background.png: approved indexed 64x64 Microchip repeat.

font/16x16_sm_ascii.png: source font used to generate FONTMAP.DAT.
palette/cx16palette.png: X16 palette used to encode the gameplay tile sheet.
palette/light.json and cyberpunk.json: theme palettes for baking banners.

scripts/: production converters, builders, and their native drawing helpers.
bitmap_font.py supplies the small HUD/banner font and tetromino shapes.
hud_art.py and nameplate_art.py draw the approved cards and player banners.
The build writes generated tile atlases and C headers into src/.

sound/: bundled music, effects and their generated headers; see README.txt
inside that directory. Refresh them with make import-sounds.

Approved banner reference crops used by emulator tests live separately in
tests/fixtures/banners/. Tests do not write screenshots into assets/.
