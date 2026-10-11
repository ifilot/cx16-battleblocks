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
    install_pads = controller.ControllerTests.install_pads
    pad = controller.ControllerTests.pad
    tap = controller.ControllerTests.tap

    def test_setting_bounds_cycle_controller_input_and_session_retention(self):
        self.assertEqual(self.read('_ai_difficulty'), bytes([0]))
        music = self.read('_sound_music_volume')
        effects = self.read('_sound_effects_volume')
        self.install_pads()
        self.press('6'); self.press('Return')
        self.tap(2, low=4); self.tap(2, low=4)
        self.assertEqual(self.tile(5, 16), 34)
        self.assert_menu_text(16, 'EASY', x=23)
        self.tap(1, low=2)
        self.assertEqual(self.read('_ai_difficulty'), bytes([0]))
        for label in ('NORMAL', 'HARD', 'HARD'):
            self.tap(1, low=1)
            self.assert_menu_text(16, label, x=23)
        self.tap(2, high=128)
        self.assertEqual(self.read('_ai_difficulty'), bytes([0]))
        self.press('Space'); self.press('Escape')
        self.start(2); self.press('Escape')
        self.press('6'); self.press('Return')
        self.assertEqual(self.read('_ai_difficulty'), bytes([1]))
        self.assert_menu_text(16, 'NORMAL', x=23)
        self.assertEqual(self.read('_sound_music_volume'), music)
        self.assertEqual(self.read('_sound_effects_volume'), effects)

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
