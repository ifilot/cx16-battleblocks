CC=cl65
EMU=~/emulator-linux/x16emu

make: BBLOCKS.PRG

BBLOCKS.PRG: *.c *.s TILES.DAT TITLE.DAT FONTMAP.DAT
	$(CC) -O -o BBLOCKS.PRG -t cx16 -C rom.cfg *.c *.s

run: BBLOCKS.PRG TILES.DAT
	$(EMU) -prg BBLOCKS.PRG -run

TILES.DAT: ../assets/png/playscreen.png
	python3 ../assets/scripts/convert_tilemap.py playscreen.png TILES.DAT

TITLE.DAT: ../assets/png/titlescreen.png
	python3 ../assets/scripts/convert_titlescreen.py playscreen.png TITLE.DAT

FONTMAP.DAT: ../assets/scripts/fontmap.py 
	python3 ../assets/scripts/fontmap.py FONTMAP.DAT

clean:
	rm -v *.o *.DAT *.PRG