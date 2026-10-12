"""Capture native README screenshots using the sibling X16 AgentBridge."""
import os
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'x16-emulator/api/python'))
from x16agent import X16Agent


def press(emu, key):
    emu.request('key', key=key, down=True)
    emu.advance(3)
    emu.request('key', key=key, down=False)
    emu.advance(33)
    settle(emu)


def settle(emu):
    deadline = time.monotonic() + 30
    while emu.read_memory(0x9F29)[0] & 0x40:
        if time.monotonic() >= deadline:
            raise RuntimeError('Scene transition did not finish')
        emu.advance(5)
        time.sleep(0.01)


def main():
    destination = ROOT / 'docs/screenshots'
    destination.mkdir(parents=True, exist_ok=True)
    emu = X16Agent(os.environ.get('EMU', ROOT.parent / 'x16-emulator/build/x16emu'),
                   os.environ.get('ROM', ROOT.parent / 'emulator/rom.bin'), fsroot=ROOT / 'src',
                   options=['-prg', str(ROOT / 'src/BBLOCKS.PRG'), '-run'])
    try:
        labels = {name.removeprefix('.'): int(address, 16)
                  for _, address, name in (line.split() for line in (ROOT / 'src/BBLOCKS.sym').read_text().splitlines())}
        emu.advance(180)
        deadline = time.monotonic() + 30
        while (emu.read_memory(labels['_sound_ready']) != bytes([1]) or
               emu.read_memory(0x9F29)[0] & 0x7F != 0x11):
            if time.monotonic() >= deadline:
                raise RuntimeError('Splash did not finish loading')
            emu.advance(30)
            time.sleep(0.01)
        emu.advance(30)
        assert emu.screenshot(destination / 'splash.png')['captured']
        press(emu, 'Space')
        emu.advance(30)
        assert emu.screenshot(destination / 'menu.png')['captured']
        for key, name in (('6', 'options'), ('7', 'about')):
            press(emu, key)
            press(emu, 'Return')
            assert emu.screenshot(destination / f'{name}.png')['captured']
            press(emu, 'Escape')
        for theme in ('light', 'cyberpunk', 'foundry', 'synthwave', 'matrix'):
            display = 'arcade' if theme == 'light' else theme
            press(emu, '2')
            if theme != 'light':
                assert emu.screenshot(destination / f'{display}-menu.png')['captured']
            press(emu, '5')
            press(emu, 'Return')
            assert emu.screenshot(destination / f'{display}-styles.png')['captured']
            press(emu, 'Escape')
            press(emu, '4')
            press(emu, 'Return')
            emu.advance(1500)
            assert emu.screenshot(destination / f'{theme}-battle.png')['captured']
            press(emu, 'Escape')
            press(emu, 'Y')  # Confirm the quit dialog.
            if theme != 'matrix':
                press(emu, 'T')
    finally:
        emu.close()
    print(f'Saved emulator screenshots to {destination}')


if __name__ == '__main__':
    main()
