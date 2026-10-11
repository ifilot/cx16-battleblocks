CC=cl65

V ?= 0
ifeq ($(V),1)
Q :=
ASSET_OUTPUT :=
else
Q := @
ASSET_OUTPUT := > /dev/null
endif

.PHONY: make run clean

SOUND_FILES=TITLE.ZSM BATTLE.ZSM BATTLE2.ZSM BATTLE3.ZSM BATTLE4.ZSM VICTORY.ZSM GAMEOVER.ZSM SFX.BIN

make: BBLOCKS.PRG
	@:

BBLOCKS.PRG: *.c *.h *.s *.inc rom.cfg version_generated.h hud_generated.h banner_generated.h menu_title_generated.h MATRIX.DAT MATRIXP.DAT MATBG.DAT HUD-M.DAT NAME-M.DAT NAME-MX.DAT SYNTH.DAT SYNTHP.DAT HUD-S.DAT NAME-S.DAT NAME-SX.DAT MENU.DAT MENU-A.DAT ARCADEP.DAT CYBERP.DAT FOUNDRYP.DAT FOUNDRY.DAT FORGE.DAT ARCADE.DAT NAME-L.DAT NAME-F.DAT NAME-FX.DAT NAME-LX.DAT NAME-C.DAT NAME-CX.DAT HUD.DAT HUD-A.DAT HUD-F.DAT CYBER.DAT TILES.DAT TITLE.DAT TITLEMAP.DAT TITLEPAL.DAT FONTMAP.DAT $(SOUND_FILES) music.h sfx.h zsmkit.lib
	@printf '  CC      BBLOCKS.PRG\n'
	$(Q)$(CC) -O -o BBLOCKS.PRG -Ln BBLOCKS.sym -m BBLOCKS.map -t cx16 -C rom.cfg *.c *.s zsmkit.lib

run:
	$(Q)$(MAKE) --no-print-directory -C .. run

version_generated.h: ../VERSION ../tools/generate_version.py ../tools/package_game.py
	@printf '  VERSION game metadata\n'
	$(Q)python3 ../tools/generate_version.py

TILES.DAT: ../assets/png/playscreen.png ../assets/scripts/convert_tilemap.py ../assets/palette/cx16palette.png
	@printf '  ASSET   TILES.DAT\n'
	$(Q)python3 ../assets/scripts/convert_tilemap.py playscreen.png TILES.DAT $(ASSET_OUTPUT)

CYBER.DAT: ../assets/scripts/build_cyberpunk.py ../assets/png/cyberpunk-background.png
	@printf '  ASSET   CYBER.DAT\n'
	$(Q)python3 ../assets/scripts/build_cyberpunk.py $(ASSET_OUTPUT)

ARCADEP.DAT CYBERP.DAT FOUNDRYP.DAT FOUNDRY.DAT FORGE.DAT ARCADE.DAT &: TILES.DAT ../assets/scripts/build_foundry.py ../assets/palette/light.json ../assets/palette/cyberpunk.json
	@printf '  ASSET   scene palettes and Foundry tiles\n'
	$(Q)python3 ../assets/scripts/build_foundry.py $(ASSET_OUTPUT)

SYNTH.DAT SYNTHP.DAT ../assets/palette/synthwave.json &: ARCADE.DAT ../assets/scripts/build_synthwave.py ../assets/scripts/build_foundry.py ../assets/palette/light.json
	@printf '  ASSET   Synthwave palette and frames\n'
	$(Q)python3 ../assets/scripts/build_synthwave.py $(ASSET_OUTPUT)

MENU.DAT MENU-A.DAT menu_title_generated.h &: ../assets/scripts/build_menu_title.py ../assets/scripts/render_splash.py ../assets/scripts/bitmap_font.py
	@printf '  ASSET   menu title\n'
	$(Q)python3 ../assets/scripts/build_menu_title.py $(ASSET_OUTPUT)

HUD.DAT HUD-A.DAT HUD-F.DAT HUD-S.DAT HUD-M.DAT hud_generated.h &: TILES.DAT ARCADE.DAT FOUNDRY.DAT SYNTH.DAT MATRIX.DAT ../assets/scripts/build_hud.py ../assets/scripts/hud_art.py ../assets/scripts/bitmap_font.py ../assets/scripts/convert_tilemap.py ../assets/palette/cx16palette.png
	@printf '  ASSET   HUD.DAT\n'
	$(Q)python3 ../assets/scripts/build_hud.py $(ASSET_OUTPUT)

NAME-L.DAT NAME-LX.DAT NAME-C.DAT NAME-CX.DAT NAME-F.DAT NAME-FX.DAT NAME-S.DAT NAME-SX.DAT NAME-M.DAT NAME-MX.DAT banner_generated.h &: TILES.DAT HUD.DAT ARCADEP.DAT FOUNDRYP.DAT SYNTHP.DAT MATRIXP.DAT ../assets/scripts/build_nameplates.py ../assets/scripts/computer_art.py ../assets/scripts/nameplate_art.py ../assets/scripts/bitmap_font.py ../assets/palette/arcade.json ../assets/palette/cyberpunk.json ../assets/palette/foundry.json ../assets/palette/synthwave.json
	@printf '  ASSET   nameplates\n'
	$(Q)python3 ../assets/scripts/build_nameplates.py $(ASSET_OUTPUT)

../assets/png/titlescreen.png: ../assets/scripts/render_splash.py ../assets/scripts/bitmap_font.py ../assets/scripts/convert_titlescreen.py
	@printf '  ASSET   titlescreen.png\n'
	$(Q)python3 ../assets/scripts/render_splash.py $(ASSET_OUTPUT)

TITLE.DAT TITLEMAP.DAT TITLEPAL.DAT &: ../assets/png/titlescreen.png ../assets/scripts/convert_titlescreen.py
	@printf '  ASSET   title graphics\n'
	$(Q)python3 ../assets/scripts/convert_titlescreen.py titlescreen.png TITLE.DAT TITLEMAP.DAT TITLEPAL.DAT $(ASSET_OUTPUT)

FONTMAP.DAT: ../assets/scripts/fontmap.py ../assets/font/16x16_sm_ascii.png
	@printf '  ASSET   FONTMAP.DAT\n'
	$(Q)python3 ../assets/scripts/fontmap.py FONTMAP.DAT $(ASSET_OUTPUT)

clean:
	@printf '  CLEAN   generated game files\n'
	$(Q)rm -f *.o *.DAT *.PRG *.ZSM SFX.BIN BBLOCKS.sym BBLOCKS.map version_generated.h

$(SOUND_FILES) music.h sfx.h: %: ../assets/sound/%
	@printf '  COPY    %s\n' "$@"
	$(Q)cp $< $@

MATRIX.DAT MATRIXP.DAT MATBG.DAT ../assets/palette/matrix.json &: ARCADE.DAT ../assets/scripts/build_matrix.py ../assets/scripts/build_foundry.py ../assets/palette/light.json
	@printf '  ASSET   Matrix palette and frames\n'
	$(Q)python3 ../assets/scripts/build_matrix.py $(ASSET_OUTPUT)
