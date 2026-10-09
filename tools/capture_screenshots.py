"""Capture native README screenshots using the sibling X16 AgentBridge."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'x16-emulator/api/python'))
from x16agent import X16Agent


def press(emu, key):
    emu.request('key', key=key, down=True)
    emu.advance(3)
    emu.request('key', key=key, down=False)
    emu.advance(3)


def main():
    destination = ROOT / 'docs/screenshots'
    destination.mkdir(parents=True, exist_ok=True)
    emu = X16Agent(ROOT.parent / 'x16-emulator/build/x16emu',
                   ROOT.parent / 'emulator/rom.bin', fsroot=ROOT / 'src',
                   options=['-prg', str(ROOT / 'src/BBLOCKS.PRG'), '-run'])
    try:
        emu.advance(180)
        assert emu.screenshot(destination / 'splash.png')['captured']
        press(emu, 'Space')
        emu.advance(30)
        assert emu.screenshot(destination / 'menu.png')['captured']
        for theme in ('light', 'cyberpunk'):
            if theme == 'cyberpunk':
                press(emu, 'Escape')
                emu.advance(30)
                press(emu, 'T')
            press(emu, '4')
            press(emu, 'Return')
            emu.advance(1500)
            assert emu.screenshot(destination / f'{theme}-battle.png')['captured']
    finally:
        emu.close()
    print(f'Saved four emulator screenshots to {destination}')


if __name__ == '__main__':
    main()
