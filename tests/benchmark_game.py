"""Measure guest CPU cycles, independent of host speed, using X16 AgentBridge.

Run: PYTHONDONTWRITEBYTECODE=1 python3 tests/benchmark_game.py
The benchmark builds a temporary PRG with the same compiler options as the game.
It includes AI preparation/search and measures rendering/movement separately.
"""
import json
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'x16-emulator/api/python'))

HARNESS = r'''
#include "game.h"
struct GameState bench_player;
uint8_t bench_operation;
uint8_t bench_search_done;
void bench_begin(void) { asm("nop"); }
void bench_end(void) { asm("nop"); }
void main(void) {
    asm("sei"); // Isolate routine cost from unrelated ROM interrupt work.
    while(1) {
        bench_begin();
        switch(bench_operation) {
            case 0:
                ai_prepare(&bench_player, &pfeval);
                while(!ai_calculate_next_score(&bench_player, &pfeval)) {}
                break;
            case 1: game_update_shadow(&bench_player); break;
            case 2: game_piece_apply_move(&bench_player, 0, -1); break;
            case 3: screen_redraw_playfield(2, &bench_player.playfield[0][0]); break;
            case 4: game_print_score(&bench_player); break;
            case 5: ai_prepare(&bench_player, &pfeval); break;
            case 6: bench_search_done = ai_calculate_next_score(&bench_player, &pfeval); break;
            case 7: screen_draw_hud_number(0, HUD_PIECE_BASE, bench_player.piece_type); break;
        }
        bench_end();
    }
}
'''


def symbols(path):
    return {name.removeprefix('.'): int(address, 16)
            for _, address, name in (line.split() for line in path.read_text().splitlines())}


def build(directory, source_override=None):
    directory = Path(directory)
    harness = directory / 'benchmark.c'
    harness.write_text(HARNESS)
    binary = directory / 'BENCH.PRG'
    labels = directory / 'BENCH.sym'
    # Work from /tmp so cl65's intermediate files cannot disturb game objects.
    sources = []
    for source in sorted((ROOT / 'src').glob('*.c')):
        if source.name != 'main.c':
            copy = directory / source.name
            original = Path(source_override) / source.name if source_override else source
            if not original.is_file(): original = source
            copy.write_text(original.read_text())
            sources.append(str(copy))
    if source_override:
        for header in Path(source_override).glob('*.h'):
            (directory / header.name).write_text(header.read_text())
    subprocess.run(['cl65', '-O', '-I', str(ROOT / 'src'),
                    '-o', str(binary), '-Ln', str(labels), '-t', 'cx16',
                    '-C', str(ROOT / 'src/rom.cfg'), '--asm-include-dir', str(ROOT / 'src'),
                    str(harness), *sources,
                    str(ROOT / 'src/screen_asm.s'), str(ROOT / 'src/sound_asm.s'),
                    str(ROOT / 'src/input_asm.s'), str(ROOT / 'src/transition.s'),
                    str(ROOT / 'src/zsmkit.lib')], cwd=directory, check=True)
    return binary, symbols(labels)


def boards():
    empty = [[255] * 10 for _ in range(24)]
    rough = [row[:] for row in empty]
    for col, height in enumerate((3, 5, 4, 2, 5, 7, 3, 1, 4, 6)):
        for row in range(height):
            rough[row][col] = 2 if (row + col) % 7 else 255
    garbage = [row[:] for row in empty]
    for row in range(6):
        garbage[row] = [255 if col == (row * 3 + 1) % 10 else 112 + ((row + col) & 1)
                        for col in range(10)]
    tall = [row[:] for row in empty]
    for col, height in enumerate((15, 18, 16, 14, 17, 19, 13, 18, 16, 15)):
        for row in range(height):
            tall[row][col] = 3 if (row * 3 + col) % 11 else 255
    return dict(empty=empty, rough=rough, garbage=garbage, tall=tall)


START = (((0, 0), (1, 0), (2, 0), (3, 0)),
         ((1, -1), (2, -1), (1, 0), (2, 0)),
         ((2, 0), (1, 0), (3, 0), (2, -1)),
         ((1, 0), (2, 0), (2, -1), (3, -1)),
         ((1, -1), (2, -1), (2, 0), (3, 0)),
         ((1, -1), (1, 0), (2, 0), (3, 0)),
         ((1, 0), (2, 0), (3, 0), (3, -1)))


class Bench:
    def __init__(self, binary, labels):
        from x16agent import X16Agent
        self.labels = labels
        self.emu = X16Agent(ROOT.parent / 'x16-emulator/build/x16emu',
                            ROOT.parent / 'emulator/rom.bin', fsroot=binary.parent,
                            options=['-prg', str(binary), '-run'])
        self.emu.request('breakpoint', action='add', address=labels['_bench_begin'])
        result = self.emu.advance(240)
        assert result['reason'] == 'breakpoint', result

    def run(self, field, piece_type, operation=0, next_type=0):
        player = self.labels['_bench_player']
        coords = bytes(v for x, y in START[piece_type] for v in (4 + x, 20 - y))
        self.emu.write_memory(player + 9, coords)
        self.emu.write_memory(player + 49, bytes([piece_type, 0]))
        self.emu.write_memory(player + 33, bytes([next_type]) + bytes(6) + bytes([0]))
        self.emu.write_memory(player + 51, bytes(cell for row in field for cell in row))
        self.emu.write_memory(player + 291, bytes([2, 1]))
        self.emu.write_memory(player + 1, bytes([22]) * 8)
        self.emu.write_memory(player + 17, bytes([22]) * 16)
        if operation == 2:
            # Supply the valid cached landing that real vertical movement has.
            shape = [(x, -y) for x, y in START[piece_type]]
            low = min(y for _, y in shape)
            heights = [max((y + 1 for y in range(20) if field[y][x] != 255), default=0) for x in range(10)]
            landing = max(0, *(heights[4 + x] - (y - low) for x, y in shape))
            shadow = bytes(v for x, y in shape for v in (4 + x, landing + y - low))
            self.emu.write_memory(player + 25, shadow)
        self.emu.write_memory(player + 293, (123456).to_bytes(4, 'little') + (123).to_bytes(2, 'little'))
        self.emu.write_memory(self.labels['_bench_operation'], bytes([operation]))
        self.emu.request('breakpoint', action='remove', address=self.labels['_bench_begin'])
        self.emu.request('breakpoint', action='add', address=self.labels['_bench_end'])
        result = self.emu.advance(10000000, unit='cycles')
        assert result['reason'] == 'breakpoint', result
        eval_data = self.emu.read_memory(self.labels['_pfeval'], 196)
        report = dict(cycles=result['cycles'], best_rotation=eval_data[194],
                      best_offset=eval_data[195], scores=[int.from_bytes(eval_data[i:i+4], 'little')
                                                       for i in range(0, 160, 4)],
                      pieces=list(eval_data[160:192]),
                      shadow=list(self.emu.read_memory(player + 25, 8)))
        report['search_done'] = self.emu.read_memory(self.labels['_bench_search_done'])[0]
        self.emu.request('breakpoint', action='remove', address=self.labels['_bench_end'])
        self.emu.request('breakpoint', action='add', address=self.labels['_bench_begin'])
        self.emu.advance(100000, unit='cycles')
        return report

    def close(self):
        self.emu.close()


def main():
    with TemporaryDirectory(prefix='battleblocks-benchmark-') as directory:
        import argparse
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument('--source-override', type=Path, help='Use saved source files for a baseline build')
        args = parser.parse_args()
        binary, labels = build(directory, args.source_override)
        bench = Bench(binary, labels)
        try:
            report = {'mhz': bench.emu.request('status')['mhz'], 'ai': {}, 'game': {}}
            for name, board in boards().items():
                report['ai'][name] = [bench.run(board, piece_type) for piece_type in range(7)]
            for operation, name in enumerate(('shadow', 'vertical_move', 'redraw', 'score', 'ai_prepare'), 1):
                report['game'][name] = bench.run(boards()['rough'], 2, operation)['cycles']
            print(json.dumps(report, indent=2))
        finally:
            bench.close()

if __name__ == '__main__':
    main()
