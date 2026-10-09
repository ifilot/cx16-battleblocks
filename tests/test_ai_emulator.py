"""Check the optimized X16 AI against a brute-force Python placement scorer."""
from pathlib import Path
import random
import shutil
from tempfile import TemporaryDirectory
import unittest

from benchmark_game import Bench, START, boards, build, ROOT

AVAILABLE = ((ROOT.parent / 'x16-emulator/build/x16emu').is_file()
             and (ROOT.parent / 'emulator/rom.bin').is_file()
             and (ROOT.parent / 'x16-emulator/api/python/x16agent.py').is_file()
             and shutil.which('cl65') is not None)
INVALID = 0xFFFFFFFF
BUMP_WEIGHTS = (0, 1, 2, 3, 4, 8, 16, 32, 64, 75, 100, 125)


def reference_placement(field, shape, offset):
    if any(x + offset >= 10 for x, _ in shape):
        return INVALID, None
    row = 19
    while row >= 0 and all(field[row + y][offset + x] == 255 for x, y in shape):
        row -= 1
    row += 1
    if any(row + y >= 20 for _, y in shape):
        return INVALID, None
    placed = [line[:] for line in field]
    for x, y in shape:
        if placed[row + y][offset + x] != 255:
            return INVALID, None
        placed[row + y][offset + x] = 0
    lines = sum(all(cell != 255 for cell in line) for line in placed[:20])
    visible = [line for line in placed[:20] if any(cell == 255 for cell in line)]
    placed = visible + [[255] * 10 for _ in range(24 - len(visible))]
    heights = [max((y + 1 for y in range(20) if placed[y][x] != 255), default=0) for x in range(10)]
    occupied = sum(cell != 255 for line in placed[:20] for cell in line)
    holes = sum(heights) - occupied
    bump = sum(BUMP_WEIGHTS[min(abs(a - b), 11)] for a, b in zip(heights, heights[1:]))
    return 100000 + sum(heights) * 510 + holes * 869 + bump * 184 - lines * 761, placed


def reference_search(field, shapes, next_shapes):
    placements = []
    seen = set()
    for shape in shapes:
        key = tuple(sorted(shape))
        placements.extend([reference_placement(field, shape, offset) if key not in seen else (INVALID, None)
                           for offset in range(10)])
        seen.add(key)
    shortlist = sorted((i for i, (score, _) in enumerate(placements) if score != INVALID),
                       key=lambda i: placements[i][0])[:4]
    combined = {}
    for i in shortlist:
        score, board = placements[i]
        future = min(reference_placement(board, shape, offset)[0]
                     for shape in next_shapes for offset in range(10))
        combined[i] = score + (1000000 if future == INVALID else future)
    best = min(shortlist, key=combined.__getitem__) if shortlist else None
    return [score for score, _ in placements], ((255, 255) if best is None else divmod(best, 10))


@unittest.skipUnless(AVAILABLE, 'Sibling X16 AgentBridge emulator and ROM required')
class AITests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = TemporaryDirectory(prefix='battleblocks-ai-tests-')
        binary, labels = build(cls.directory.name)
        cls.bench = Bench(binary, labels)
        data = cls.bench.emu.read_memory(labels['_piece_rotation_table'], 224)
        cls.rotations = [v if v < 128 else v - 256 for v in data]

    @classmethod
    def tearDownClass(cls):
        cls.bench.close()
        cls.directory.cleanup()

    def shapes(self, piece_type):
        coords = [(4 + x, 20 - y) for x, y in START[piece_type]]
        result = []
        for rotation in range(4):
            minx = min(x for x, _ in coords)
            miny = min(y for _, y in coords)
            result.append([(x - minx, y - miny) for x, y in coords])
            base = (piece_type * 4 + rotation) * 8
            coords = [(x + self.rotations[base + 2 * i], y - self.rotations[base + 2 * i + 1])
                      for i, (x, y) in enumerate(coords)]
        return result

    def check_board(self, field, next_type=0):
        for piece_type in range(7):
            with self.subTest(piece_type=piece_type, next_type=next_type):
                actual = self.bench.run(field, piece_type, next_type=next_type)
                shapes = self.shapes(piece_type)
                self.assertEqual(actual['pieces'], [v for shape in shapes for point in shape for v in point])
                expected, best = reference_search(field, shapes, self.shapes(next_type))
                self.assertEqual(actual['scores'], expected)
                self.assertEqual((actual['best_rotation'], actual['best_offset']), best)
                field_ram = self.bench.emu.read_memory(self.bench.labels['_bench_player'] + 51, 240)
                self.assertEqual(field_ram, bytes(cell for line in field for cell in line))

    def test_every_preview_type_and_line_collapse(self):
        field = [[255] * 10 for _ in range(24)]
        for row in range(3):
            field[row] = [112 if col != 4 else 255 for col in range(10)]
        for next_type in range(7):
            self.check_board(field, next_type)

    def test_preview_changes_the_current_target(self):
        heights = (2, 3, 3, 2, 3, 1, 3, 0, 3, 5)
        field = [[2 if row < heights[col] else 255 for col in range(10)] for row in range(24)]
        targets = set()
        for next_type in range(7):
            actual = self.bench.run(field, 2, next_type=next_type)
            _, best = reference_search(field, self.shapes(2), self.shapes(next_type))
            target = (actual['best_rotation'], actual['best_offset'])
            self.assertEqual(target, best)
            targets.add(target)
        self.assertGreater(len(targets), 1)

    def test_search_yields_between_bounded_steps(self):
        field = boards()['tall']
        self.bench.run(field, 2, operation=5, next_type=2)
        costs = []
        for _ in range(180):
            result = self.bench.run(field, 2, operation=6, next_type=2)
            costs.append(result['cycles'])
            if result['search_done']:
                break
        self.assertTrue(result['search_done'])
        self.assertGreater(len(costs), 40)  # Preview expansion runs across loop turns.
        self.assertLess(max(costs), 160000)  # Under 20 ms at the X16's 8 MHz.

    def test_empty_rough_garbage_and_tall_stacks(self):
        for name, field in boards().items():
            with self.subTest(board=name):
                self.check_board(field)

    def test_high_stacks_holes_and_extreme_wells_do_not_overflow(self):
        for style in ('plateau', 'holes', 'well'):
            field = [[255] * 10 for _ in range(24)]
            for row in range(20):
                for col in range(10):
                    if (style == 'plateau' and row < 14 or
                        style == 'holes' and row == 18 or
                        style == 'well' and (row < 2 or col == 0)):
                        field[row][col] = 113
            with self.subTest(style=style):
                self.check_board(field)

    def test_shadow_projection_matches_collision_search(self):
        for name, field in boards().items():
            for piece_type in range(7):
                with self.subTest(board=name, piece_type=piece_type):
                    result = self.bench.run(field, piece_type, operation=1)
                    coords = [(4 + x, 20 - y) for x, y in START[piece_type]]
                    miny = min(y for _, y in coords)
                    shape = [(x, y - miny) for x, y in coords]
                    row = 19
                    while row >= 0 and all(field[row + y][x] == 255 for x, y in shape):
                        row -= 1
                    expected = [v for x, y in shape for v in (x, row + 1 + y)]
                    self.assertEqual(result['shadow'], expected)

    def test_random_boards_match_brute_force_search(self):
        rng = random.Random(0x6502)
        for case in range(12):
            field = [[255] * 10 for _ in range(24)]
            for col in range(10):
                for row in range(rng.randrange(19)):
                    if rng.random() < 0.8:
                        field[row][col] = rng.choice((2, 112, 113))
            with self.subTest(case=case):
                self.check_board(field)


if __name__ == '__main__':
    unittest.main()
