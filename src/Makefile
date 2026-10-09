CC=cl65
EMU=~/emulator-linux/x16emu

SOUND_FILES=TITLE.ZSM BATTLE.ZSM BATTLE2.ZSM BATTLE3.ZSM BATTLE4.ZSM VICTORY.ZSM GAMEOVER.ZSM SFX.BIN

make: BBLOCKS.PRG

BBLOCKS.PRG: *.c *.h *.s *.inc rom.cfg hud_generated.h banner_generated.h NAME-L.DAT NAME-LX.DAT NAME-C.DAT NAME-CX.DAT HUD.DAT CYBER.DAT TILES.DAT TITLE.DAT TITLEMAP.DAT TITLEPAL.DAT FONTMAP.DAT $(SOUND_FILES) music.h sfx.h zsmkit.lib
	$(CC) -O -o BBLOCKS.PRG -Ln BBLOCKS.sym -m BBLOCKS.map -t cx16 -C rom.cfg *.c *.s zsmkit.lib

run: BBLOCKS.PRG TILES.DAT
	$(EMU) -prg BBLOCKS.PRG -run

TILES.DAT: ../assets/png/playscreen.png ../assets/scripts/convert_tilemap.py ../assets/palette/cx16palette.png
	python3 ../assets/scripts/convert_tilemap.py playscreen.png TILES.DAT

CYBER.DAT: ../assets/scripts/build_cyberpunk.py ../assets/png/cyberpunk-background.png
	python3 ../assets/scripts/build_cyberpunk.py

HUD.DAT hud_generated.h &: TILES.DAT ../assets/scripts/build_hud.py ../assets/scripts/hud_art.py ../assets/scripts/bitmap_font.py ../assets/scripts/convert_tilemap.py ../assets/palette/cx16palette.png
	python3 ../assets/scripts/build_hud.py

NAME-L.DAT NAME-LX.DAT NAME-C.DAT NAME-CX.DAT banner_generated.h &: TILES.DAT HUD.DAT ../assets/scripts/build_nameplates.py ../assets/scripts/nameplate_art.py ../assets/scripts/bitmap_font.py ../assets/palette/light.json ../assets/palette/cyberpunk.json
	python3 ../assets/scripts/build_nameplates.py

../assets/png/titlescreen.png: ../assets/scripts/render_splash.py ../assets/scripts/bitmap_font.py ../assets/scripts/convert_titlescreen.py
	python3 ../assets/scripts/render_splash.py

TITLE.DAT TITLEMAP.DAT TITLEPAL.DAT &: ../assets/png/titlescreen.png ../assets/scripts/convert_titlescreen.py
	python3 ../assets/scripts/convert_titlescreen.py titlescreen.png TITLE.DAT TITLEMAP.DAT TITLEPAL.DAT

FONTMAP.DAT: ../assets/scripts/fontmap.py ../assets/font/16x16_sm_ascii.png
	python3 ../assets/scripts/fontmap.py FONTMAP.DAT

clean:
	rm -f *.o *.DAT *.PRG *.ZSM SFX.BIN BBLOCKS.sym BBLOCKS.map

$(SOUND_FILES) music.h sfx.h: %: ../assets/sound/%
	cp $< $@
