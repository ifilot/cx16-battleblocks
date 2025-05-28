CC=cl65
EMU=~/emulator-linux/x16emu

make: BBLOCKS.PRG

BBLOCKS.PRG: *.c *.s
	$(CC) -O -o BBLOCKS.PRG -t cx16 -C rom.cfg *.c *.s

run: BBLOCKS.PRG TILES.DAT
	$(EMU) -prg BBLOCKS.PRG -run

TILES.DAT: ../assets/png/playscreen.png
	python3 ../assets/scripts/convert_tilemap.py playscreen.png TILES.DAT

clean:
	rm -v *.o *.DAT *.PRG