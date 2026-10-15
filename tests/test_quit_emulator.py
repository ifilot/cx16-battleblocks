"""Exercise the quit dialog in the running game, including nested pause."""
import unittest
import test_menu_emulator as menu
import test_controller_emulator as controller


@unittest.skipUnless(menu.AVAILABLE, 'Sibling X16 AgentBridge required')
class QuitTests(unittest.TestCase):
    setUp = menu.MenuTests.setUp
    read = menu.MenuTests.read
    start = menu.MenuTests.start
    tile = menu.MenuTests.tile
    assert_cursor = menu.MenuTests.assert_cursor
    assert_menu_text = menu.MenuTests.assert_menu_text
    install_pads = controller.ControllerTests.install_pads
    pad = controller.ControllerTests.pad
    tap = controller.ControllerTests.tap

    def press(self, key, settle=True, confirm_quit=False):
        menu.MenuTests.press(self, key, settle, confirm_quit)

    def maps(self):
        return self.emu.read_memory(0x18000, 16384, space='vram')

    def boards(self):
        return [self.read(name, 0, 351) for name in ('_gs_player1', '_gs_player2')]

    def test_default_no_escape_cancel_and_frozen_match_in_every_mode(self):
        for choice in (1, 2, 3, 4):
            with self.subTest(choice=choice):
                self.start(choice)
                self.press('Escape')
                self.assertEqual(self.read('_game_quit_open'), bytes([1]))
                self.assert_menu_text(13, 'QUIT MATCH')
                self.assert_menu_text(16, '  YES   > NO')
                for key in ('Left', 'Right', 'Left', 'Right'):
                    self.press(key)
                    self.assert_menu_text(16, 'YES', x=16)
                    self.assert_menu_text(16, 'NO', x=24)
                    self.assertEqual(self.tile(14, 16), 34 if key == 'Left' else 0)
                    self.assertEqual(self.tile(22, 16), 34 if key == 'Right' else 0)
                snapshot = self.boards()
                self.emu.advance(310)
                self.assertEqual(self.boards(), snapshot)
                self.press('Escape')
                self.assertEqual(self.read('_game_quit_open'), bytes([0]))
                self.assertEqual(self.read('_game_paused'), bytes([0]))
                self.assertEqual(self.read('_game_mode'), bytes([choice - 1]))
                self.press('Escape'); self.press('Return')
                self.assertEqual(self.read('_game_quit_open'), bytes([0]))
                self.press('Escape'); self.press('Left'); self.press('Return')
                self.assertEqual(self.tile(16, 6 + 2 * choice), 34)

    def test_cancel_restores_maps_pause_state_and_has_stack_headroom(self):
        self.install_pads(); self.start(3)
        self.tap(1, low=16)
        snapshot, maps = self.boards(), self.maps()
        stack_bottom = self.symbols['__HIMEM__'] - self.symbols['__STACKSIZE__']
        guard = bytes([0xa5]) * 128
        self.emu.write_memory(stack_bottom, guard)
        for cancel in ('Escape', 'N', 'Return'):
            self.press('Escape')
            self.emu.advance(310)
            self.assertEqual(self.boards(), snapshot)
            self.press(cancel)
            self.assertEqual(self.maps(), maps)
            self.assertEqual(self.boards(), snapshot)
            self.assertEqual(self.read('_game_paused'), bytes([1]))
        self.assertEqual(self.emu.read_memory(stack_bottom, 128), guard)
        self.tap(2, low=16)
        self.assertEqual(self.read('_game_paused'), bytes([0]))

    def test_controller_can_cancel_or_confirm_and_paused_game_can_quit(self):
        self.install_pads(); self.start(3)
        self.press('Escape'); self.tap(2, low=32)
        self.assertEqual(self.read('_game_quit_open'), bytes([0]))
        self.press('Escape'); self.tap(1, low=2)
        self.assert_menu_text(16, '> YES     NO')
        self.tap(2, high=128)
        self.assertEqual(self.tile(16, 12), 34)
        self.start(3); self.tap(1, low=16)
        self.press('Escape'); self.press('Y')
        self.assertEqual(self.read('_game_paused'), bytes([0]))
        self.assertEqual(self.tile(16, 12), 34)

    def test_exit_item_confirms_then_returns_to_a_clean_basic_prompt(self):
        self.press('8')
        self.assert_cursor(22)
        self.assert_menu_text(25, 'RETURN TO THE BASIC PROMPT')
        self.assert_menu_text(28, 'UP DOWN OR 1 TO 8 SELECT  ENTER OPEN')
        maps = self.maps()
        for cancel in ('Escape', 'N', 'Return'):  # Return confirms the default No.
            self.press('Return')
            self.assertEqual(self.read('_game_quit_open'), bytes([1]))
            self.assert_menu_text(13, 'EXIT TO BASIC')
            self.assert_menu_text(16, '  YES   > NO')
            self.press(cancel)
            self.assertEqual(self.read('_game_quit_open'), bytes([0]))
            self.assertEqual(self.maps(), maps)
        self.press('Return'); self.press('Left'); self.press('Return')
        self.emu.advance(60)
        # The KERNAL owns the IRQ again and every PSG voice is silent.
        self.assertGreaterEqual(int.from_bytes(self.emu.read_memory(0x314, 2), 'little'), 0xC000)
        psg = self.emu.read_memory(0x1F9C0, 64, space='vram')
        self.assertEqual([psg[i] & 0x3F for i in range(2, 64, 4)], [0] * 16)
        # Default text palette: black, white, red, cyan.
        self.assertEqual(self.emu.read_memory(0x1FA00, 8, space='vram'), bytes.fromhex('0000ff0f0008fe0a'))
        def row(y):
            cells = self.emu.read_memory(0x1B000 + y * 256, 12, space='vram')
            return bytes(cells[::2])
        ready = bytes([0x12, 0x05, 0x01, 0x04, 0x19, 0x2E])  # READY. in screen codes
        self.assertEqual(row(1)[:6], ready)


if __name__ == '__main__':
    unittest.main()
