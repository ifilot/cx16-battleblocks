"""Exercise landing shadows while sliding beneath an overhang in the real game."""
import unittest

import test_menu_emulator as menu


@unittest.skipUnless(menu.AVAILABLE, 'Sibling X16 AgentBridge emulator and ROM required')
class ShadowTests(unittest.TestCase):
    setUp = menu.MenuTests.setUp
    press = menu.MenuTests.press
    read = menu.MenuTests.read
    write = menu.MenuTests.write
    start = menu.MenuTests.start
    tile = menu.MenuTests.tile

    def prepare_overhang(self):
        self.start(1)
        field = bytearray([255] * 240)
        field[4 * 10 + 3:4 * 10 + 5] = bytes([2, 2])
        self.write('_gs_player1', 51, field)
        coords = bytes([1, 1, 2, 1, 1, 2, 2, 2])
        shadow = bytes([1, 0, 2, 0, 1, 1, 2, 1])
        self.write('_gs_player1', 1, coords + coords + shadow + shadow)
        self.write('_gs_player1', 49, bytes([1, 0]))  # O piece.
        for row in range(20):
            tiles = bytearray(20)
            for col in range(10):
                if field[row * 10 + col] != 255:
                    tiles[col * 2] = 16 + field[row * 10 + col]
            self.emu.write_memory(0x1A000 + ((27 - row) * 64 + 1) * 2, tiles, space='vram')
        for cells, tile in ((shadow, 23), (coords, 17)):
            for x, y in zip(cells[::2], cells[1::2]):
                self.emu.write_memory(0x1A000 + ((27 - y) * 64 + 1 + x) * 2,
                                      bytes([tile, 0]), space='vram')
        self.press('Right')
        self.press('Right')

    def assert_no_stray_shadows(self):
        shadow = self.read('_gs_player1', 25, 8)
        expected = set(zip(shadow[::2], shadow[1::2]))
        stray = [(x, y) for y in range(20) for x in range(10)
                 if self.tile(1 + x, 27 - y) == 23 and (x, y) not in expected]
        self.assertEqual(stray, [], 'Shadow tiles survived after the next piece spawned')

    def test_shadow_stays_below_piece_slid_under_overhang(self):
        self.prepare_overhang()
        self.assertEqual(self.read('_gs_player1', 25, 8), bytes([3, 0, 4, 0, 3, 1, 4, 1]))

    def test_soft_lock_under_overhang_leaves_no_old_shadow(self):
        self.prepare_overhang()
        self.press('Down')
        self.press('Down')
        self.assertEqual(self.read('_gs_player1', 51 + 3, 2), bytes([1, 1]))
        self.assert_no_stray_shadows()

    def test_hard_drop_under_overhang_locks_into_gap(self):
        self.prepare_overhang()
        self.press('Space')
        self.assertEqual(self.read('_gs_player1', 51 + 3, 2), bytes([1, 1]))
        self.assertEqual(self.read('_gs_player1', 51 + 13, 2), bytes([1, 1]))
        self.assert_no_stray_shadows()
