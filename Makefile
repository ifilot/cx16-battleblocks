EMU ?= $(or $(firstword $(wildcard ../x16-emulator/build/x16emu $(HOME)/emulator-linux/x16emu)),$(abspath ../x16-emulator/build/x16emu))
ROM ?= $(or $(firstword $(wildcard $(dir $(EMU))rom.bin ../emulator/rom.bin $(HOME)/emulator-linux/rom.bin)),$(abspath ../emulator/rom.bin))
EMU_FLAGS ?= -scale 2 -joy1 -joy2
DEMO_GIF ?= img/cx16-battleblocks-gameplay.gif
DEMO_CYBERPUNK_GIF ?= img/cx16-battleblocks-cyberpunk-gameplay.gif
SOUND_SOURCE ?= $(abspath ../cx16-sound-generator/build/battleblocks/psg)

# Use V=1 to show recipe commands while keeping diagnostics visible by default.
V ?= 0
ifeq ($(V),1)
Q :=
else
Q := @
endif

.PHONY: all run demo check-run clean import-sounds dist site

all:
	@printf '  BUILD   Battle Blocks\n'
	$(Q)$(MAKE) --no-print-directory -C src
	@printf '  READY   src/BBLOCKS.PRG\n'

run: check-run
	$(Q)$(MAKE) --no-print-directory all
	$(Q)emu="$(EMU)"; rom="$(ROM)"; \
		case "$$emu" in /*) ;; *) emu="$$PWD/$$emu" ;; esac; \
		case "$$rom" in /*) ;; *) rom="$$PWD/$$rom" ;; esac; \
		printf '  RUN     Battle Blocks\n  EMU     %s\n  ROM     %s\n' "$$emu" "$$rom"; \
		cd src && "$$emu" -rom "$$rom" -prg BBLOCKS.PRG -run $(EMU_FLAGS)

check-run:
	@if [ ! -x "$(EMU)" ]; then \
		printf 'Error: X16 emulator is missing or not executable: %s\nSet its location with make run EMU=/path/to/x16emu\n' "$(EMU)" >&2; exit 1; \
	fi
	@if [ ! -f "$(ROM)" ] || [ ! -r "$(ROM)" ]; then \
		printf 'Error: X16 ROM is missing or unreadable: %s\nSet its location with make run ROM=/path/to/rom.bin\n' "$(ROM)" >&2; exit 1; \
	fi

demo: check-run
	$(Q)$(MAKE) --no-print-directory all
	$(Q)python3 tools/record_gameplay.py --emulator "$(EMU)" --rom "$(ROM)" \
		--output "$(DEMO_GIF)" --cyberpunk-output "$(DEMO_CYBERPUNK_GIF)"

clean:
	$(Q)$(MAKE) --no-print-directory -C src clean

import-sounds:
	$(Q)python3 assets/scripts/import_sounds.py --source "$(SOUND_SOURCE)"

dist: all
	$(Q)python3 tools/package_game.py

site: dist
	$(Q)python3 tools/build_site.py
