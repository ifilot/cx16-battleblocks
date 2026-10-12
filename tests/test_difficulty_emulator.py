"""Check session settings and the computer's actual action cadence."""
import unittest
import test_menu_emulator as menu
import test_controller_emulator as controller


@unittest.skipUnless(menu.AVAILABLE, 'Sibling X16 AgentBridge required')
class DifficultyTests(unittest.TestCase):
    setUp = menu.MenuTests.setUp
    press = menu.MenuTests.press
    read = menu.MenuTests.read
    write = menu.MenuTests.write
    start = menu.MenuTests.start
    tile = menu.MenuTests.tile
    assert_menu_text = menu.MenuTests.assert_menu_text
    assert_cursor = menu.MenuTests.assert_cursor
    install_pads = controller.ControllerTests.install_pads
    pad = controller.ControllerTests.pad
    tap = controller.ControllerTests.tap

    def attribute(self, x, y):
        return self.emu.read_memory(0x1A000 + (y * 64 + x) * 2 + 1, space='vram')[0]

    def assert_difficulty(self, level):
        labels = (' EASY ', 'NORMAL', ' HARD ')
        self.assertEqual(self.read('_ai_difficulty'), bytes([level]))
        self.assert_menu_text(4, 'COMPUTER', x=4)
        self.assert_menu_text(5, labels[level], x=5)
        # The left arrow is the selection cursor, flipped horizontally.
        self.assertEqual((self.tile(4, 5), self.attribute(4, 5)),
                         (34, 4) if level else (0, 4))
        self.assertEqual(self.tile(11, 5), 34 if level < 2 else 0)
        # The 2x2 computer face leads the banner atlas at tile 316.
        for index, (x, y) in enumerate(((1, 4), (2, 4), (1, 5), (2, 5))):
            self.assertEqual((self.tile(x, y), self.attribute(x, y)), (60 + index, 1))

    def test_main_menu_bounds_controller_input_and_session_retention(self):
        self.assert_difficulty(0)
        music = self.read('_sound_music_volume')
        effects = self.read('_sound_effects_volume')
        self.install_pads()
        self.tap(1, low=2)
        self.assert_difficulty(0)
        for level in (1, 2, 2):
            self.tap(2, low=1)
            self.assert_difficulty(level)
        self.press('A')
        self.assert_difficulty(1)
        # Changing difficulty keeps the selected menu entry.
        self.assert_cursor(8)
        self.start(2); self.press('Escape')
        self.assert_difficulty(1)
        self.press('Left'); self.press('Right'); self.press('D')
        self.assert_difficulty(2)
        # Options now hold only audio settings.
        self.press('6'); self.press('Return')
        self.assert_menu_text(16, 'BACK', x=7)
        self.press('Escape')
        self.assert_difficulty(2)
        self.assertEqual(self.read('_sound_music_volume'), music)
        self.assertEqual(self.read('_sound_effects_volume'), effects)

    def test_computer_face_follows_theme_and_survives_games(self):
        atlases = ('NAME-L.DAT', 'NAME-C.DAT', 'NAME-F.DAT', 'NAME-S.DAT', 'NAME-M.DAT')
        for style, atlas in enumerate(atlases):
            if style:
                self.press('T')
            expected = (menu.ROOT / 'src' / atlas).read_bytes()[:4 * 256]
            for _ in range(2):
                self.assertEqual(self.read('_screen_style'), bytes([style]))
                self.assertEqual(self.emu.read_memory(316 * 256, 4 * 256, space='vram'), expected)
                self.assert_difficulty(0)
                self.start(2); self.press('Escape')

    def test_drop_wait_and_movement_rate_for_each_difficulty(self):
        for difficulty, move_ticks, drop_ticks in ((0, 12, 60), (1, 6, 30), (2, 2, 2)):
            with self.subTest(difficulty=difficulty):
                # Keep the opening spawn idle until its controlled plan is set.
                self.write('_ai_difficulty', 0, bytes([0]))
                self.start(2)
                # A finished plan with the current O already at its target.
                self.write('_gs_player2', 9, bytes((4, 18, 5, 18, 4, 19, 5, 19)))
                self.write('_gs_player2', 25, bytes((4, 0, 5, 0, 4, 1, 5, 1)))
                self.write('_gs_player2', 49, bytes([1, 0]))
                self.write('_pfeval', 192, bytes([4, 0, 0, 4]))
                self.write('_pfeval', 355, bytes([2]))
                self.write('_ai_difficulty', 0, bytes([difficulty]))
                before = self.read('_gs_player2', 51, 240)
                if difficulty < 2:
                    self.emu.advance(drop_ticks // 2)
                    self.assertEqual(self.read('_gs_player2', 51, 240), before)
                self.emu.advance(drop_ticks + 10)
                self.assertNotEqual(self.read('_gs_player2', 51, 240), before)
                # Eight columns away from the target: steer, rather than drop.
                self.write('_gs_player2', 9, bytes((8, 18, 9, 18, 8, 19, 9, 19)))
                self.write('_gs_player2', 49, bytes([1, 0]))
                self.write('_pfeval', 192, bytes([0, 0, 0, 0]))
                self.write('_pfeval', 355, bytes([2]))
                self.emu.advance(12)
                moved = 8 - self.read('_gs_player2', 9)[0]
                self.assertGreaterEqual(moved, 1)
                self.assertLessEqual(moved, 12 // move_ticks + 1)
                self.press('Escape')

    def test_slower_levels_spawn_fewer_pieces(self):
        counts = []
        for difficulty in (0, 1, 2):
            self.write('_ai_difficulty', 0, bytes([difficulty]))
            self.start(2)
            self.emu.advance(600)
            data = self.read('_gs_player2', 326, 14)
            counts.append(sum(int.from_bytes(data[i:i+2], 'little') for i in range(0, 14, 2)))
            self.press('Escape')
        self.assertLess(counts[0], counts[1], counts)
        self.assertLess(counts[1], counts[2], counts)


if __name__ == '__main__':
    unittest.main()
