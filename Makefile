EMU ?= $(abspath ../x16-emulator/build/x16emu)
ROM ?= $(abspath ../emulator/rom.bin)
EMU_FLAGS ?= -scale 2
SOUND_SOURCE ?= $(abspath ../cx16-sound-generator/build/battleblocks/psg)

.PHONY: all run clean import-sounds dist site

all:
	$(MAKE) -C src

run: all
	cd src && "$(EMU)" -rom "$(ROM)" -prg BBLOCKS.PRG -run $(EMU_FLAGS)

clean:
	$(MAKE) -C src clean

import-sounds:
	python3 assets/scripts/import_sounds.py --source "$(SOUND_SOURCE)"

dist: all
	python3 tools/package_game.py

site: dist
	python3 tools/build_site.py
