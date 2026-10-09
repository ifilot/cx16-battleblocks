"""Refresh the bundled Battle Blocks audio from chipscore's generated output."""
import argparse
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[2]
SONGS = ('TITLE', 'BATTLE', 'BATTLE2', 'BATTLE3', 'BATTLE4', 'VICTORY', 'GAMEOVER')


def import_sounds(source):
    source = Path(source)
    music = (source / 'music.h').read_text()
    sfx = (source / 'sfx.h').read_text()
    occupied = {1, 2}
    files = [name + '.ZSM' for name in SONGS] + ['SFX.BIN', 'music.h', 'sfx.h']
    for name in SONGS:
        data = (source / (name + '.ZSM')).read_bytes()
        assert data[:3] == b'zm\x01', f'{name}: invalid ZSM header'
        bank = int(re.search(rf'^#define MUSIC_{name}_BANK (\d+)', music, re.M)[1])
        banks = set(range(bank, bank + (len(data) + 8191) // 8192))
        assert bank >= 3 and max(banks) <= 255 and not occupied & banks, f'{name}: bank overlap'
        occupied |= banks
    effects = (source / 'SFX.BIN').read_bytes()
    assert len(effects) <= 8192, 'Effects must fit in bank 2'
    assert len(effects) == int(re.search(r'^#define SFX_BANK_SIZE (\d+)', sfx, re.M)[1])
    for offset in re.findall(r'^#define SFX_\w+\s+(0x[0-9A-F]+)\s+/\*', sfx, re.M):
        start = int(offset, 16)
        assert effects[start:start + 3] == b'zm\x01', f'Invalid effect at {offset}'
    destination = ROOT / 'assets/sound'
    destination.mkdir(exist_ok=True)
    for name in files:
        shutil.copyfile(source / name, destination / name)
    print(f'Imported {len(SONGS)} songs and the effect bank from {source}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path,
                        default=ROOT.parent / 'cx16-sound-generator/build/battleblocks/psg')
    import_sounds(parser.parse_args().source)
