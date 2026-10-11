"""Record Commander X16 and Cyberpunk gameplay GIFs from real X16 AgentBridge input."""
import argparse
from pathlib import Path
import re
import shutil
import sys
import tempfile

from PIL import Image

from package_game import RUNTIME_FILES

ROOT = Path(__file__).resolve().parents[1]
FPS = 10


def encode(frames, output):
    # Use one palette for the entire clip to keep the native pixel art stable.
    samples = Image.new('RGB', (160 * 8, 120 * ((len(frames) + 7) // 8)))
    for index, frame in enumerate(frames):
        samples.paste(frame.resize((160, 120), Image.Resampling.NEAREST),
                      ((index % 8) * 160, (index // 8) * 120))
    palette = samples.quantize(colors=256, dither=Image.Dither.NONE)
    indexed = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
    output.parent.mkdir(parents=True, exist_ok=True)
    indexed[0].save(output, save_all=True, append_images=indexed[1:],
                    duration=100, loop=0, optimize=True, disposal=1)
    with Image.open(output) as gif:
        assert gif.size == (640, 480), gif.size
        assert gif.info['loop'] == 0
        duration = 0
        for index in range(gif.n_frames):
            gif.seek(index)
            delay = gif.info['duration']
            assert delay >= 100 and delay % 100 == 0, delay
            duration += delay
        assert duration == len(frames) * 100, duration
    print(f'  SAVED   {output} ({duration / 1000:.1f}s, 640x480, {FPS} fps)', flush=True)


def record(agent_class, emulator, rom, symbols, theme, output):
    frames = []
    label = 'Commander X16' if theme == 0 else 'Cyberpunk'
    with tempfile.TemporaryDirectory(prefix='battleblocks-demo-') as directory:
        runtime = Path(directory)
        for name in RUNTIME_FILES:
            shutil.copy2(ROOT / 'src' / name, runtime / name)
        with agent_class(emulator, rom, fsroot=runtime,
                         options=['-prg', str(runtime / 'BBLOCKS.PRG'), '-run']) as emu:
            def read(name, offset=0, size=1):
                return emu.read_memory(symbols['_' + name] + offset, size)

            def capture(count):
                for _ in range(count):
                    # Screenshot capture advances two frames; four more give
                    # six emulated frames per sample (10 fps at native speed).
                    emu.advance(4)
                    result = emu.screenshot(runtime / 'frame.png')
                    assert result['captured'], result
                    with Image.open(runtime / 'frame.png') as frame:
                        assert frame.size == (640, 480), frame.size
                        frames.append(frame.convert('RGB'))

            def key(name):
                emu.request('key', key=name, down=True)
                capture(1)
                emu.request('key', key=name, down=False)
                capture(1)

            print(f'  DEMO    {label}: title and menu', flush=True)
            emu.advance(180)
            capture(20)
            key('Space')
            capture(10)
            while read('screen_style') != bytes([theme]):
                key('T')
                capture(10)
            assert read('screen_style') == bytes([theme]), 'Theme selection failed'
            key('4')
            capture(10)
            key('Return')
            emu.advance(30)
            assert read('game_mode') == bytes([3]), 'Computer demo did not start'
            # Wait for artwork loading to finish.
            for _ in range(120):
                if emu.read_memory(0x9F29)[0] & 0x7F == 0x31:
                    break
                emu.advance(1)
            else:
                raise RuntimeError('Gameplay display did not finish loading')

            print(f'  DEMO    {label}: computer-versus-computer gameplay', flush=True)
            capture(30 * FPS)
            for player in ('gs_player1', 'gs_player2'):
                assert read(player, offset=51, size=240) != bytes([255]) * 240, \
                    f'{player} did not place any pieces'
            assert read('screen_style') == bytes([theme]), 'Gameplay theme changed'
            key('Escape')
            capture(15)
            key('Escape')
            capture(10)

    print(f'  GIF     {label}: encoding', flush=True)
    encode(frames, output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emulator', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--api-dir', type=Path,
                        help='Directory containing the AgentBridge x16agent.py client')
    parser.add_argument('--output', type=Path, default=ROOT / 'img/cx16-battleblocks-gameplay.gif')
    parser.add_argument('--cyberpunk-output', type=Path,
                        default=ROOT / 'img/cx16-battleblocks-cyberpunk-gameplay.gif')
    args = parser.parse_args()
    if args.output.resolve() == args.cyberpunk_output.resolve():
        parser.error('The two demos must have different output paths')
    emulator, rom = args.emulator.resolve(), args.rom.resolve()
    api = args.api_dir or emulator.parent.parent / 'api/python'
    sys.path.insert(0, str(api))
    try:
        from x16agent import X16Agent
    except ImportError:
        parser.error('AgentBridge Python client not found; supply --api-dir')
    symbols = {name: int(address, 16) for address, name in re.findall(
        r'al ([0-9A-Fa-f]+) \.(\S+)', (ROOT / 'src/BBLOCKS.sym').read_text())}
    record(X16Agent, emulator, rom, symbols, 0, args.output)
    record(X16Agent, emulator, rom, symbols, 1, args.cyberpunk_output)


if __name__ == '__main__':
    main()
