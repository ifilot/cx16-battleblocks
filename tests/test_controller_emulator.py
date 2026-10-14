"""Exercise real scene loops with injected active-low KERNAL SNES states.

AgentBridge has no controller injection. Replace only the joystick_get call
with a RAM stub; leave clock interrupts, gameplay, menus and polling intact.
"""
import unittest
import test_menu_emulator as menu


@unittest.skipUnless(menu.AVAILABLE, "Sibling X16 AgentBridge required")
class ControllerTests(unittest.TestCase):
    setUp = menu.MenuTests.setUp
    press = menu.MenuTests.press
    read = menu.MenuTests.read
    start = menu.MenuTests.start

    def install_pads(self):
        # Raw bytes returned in A/X/Y for each physical port.
        self.emu.write_memory(0x600, bytes([255,255,255,255,0,0]))
        # index = port - 1; preserve A while loading X and presence in Y.
        stub = bytes.fromhex('3a aa bd0006 48 bd0406 a8 bd0206 aa 68 60')
        self.emu.write_memory(0x500, stub)
        address = self.symbols['_input_controller'] + 3
        self.assertEqual(self.emu.read_memory(address,3), bytes.fromhex('2056ff'))
        self.emu.write_memory(address, bytes.fromhex('200005'))

    def pad(self, port, low=0, high=0, frames=3, present=True, settle=True):
        self.emu.write_memory(0x600+port-1, bytes([255 ^ low]))
        self.emu.write_memory(0x602+port-1, bytes([255 ^ high]))
        self.emu.write_memory(0x604+port-1, bytes([0 if present else 255]))
        self.emu.advance(frames)
        if not settle: return
        deadline = menu.time.monotonic()+30
        while self.emu.read_memory(0x9F29)[0]&0x40:
            if menu.time.monotonic()>deadline: self.fail('Controller scene timeout')
            self.emu.advance(5)
            menu.time.sleep(.01)

    def tap(self, port, low=0, high=0):
        self.pad(port,low,high,frames=1,settle=False)
        self.pad(port)

    def test_both_controllers_navigate_apply_and_cancel_menus(self):
        self.install_pads()
        self.assertEqual(self.tile(16,8),34)
        self.tap(2,low=8)  # Up wraps from solo to About.
        self.assertEqual(self.tile(16,20),34)
        self.tap(2,low=4)
        self.assertEqual(self.tile(16,8),34)
        self.tap(1,low=4)
        self.assertEqual(self.tile(16,10),34)
        self.press('5'); self.tap(2,high=128)
        self.tap(1,low=4)
        self.assertEqual(self.read('_screen_style'),bytes([1]))
        self.tap(2,low=32)  # Select cancels preview.
        self.assertEqual(self.read('_screen_style'),bytes([0]))
        self.tap(1,low=16)
        self.tap(2,low=4)
        self.tap(2,high=64)  # X confirms theme.
        self.assertEqual(self.read('_screen_style'),bytes([1]))

    tile = menu.MenuTests.tile

    def test_local_players_are_independent_and_buttons_do_not_repeat(self):
        self.install_pads(); self.start(3)
        x=self.read('_gs_player1',9)[0]; other=self.read('_gs_player2',9)[0]
        self.tap(1,low=1)
        self.assertEqual(self.read('_gs_player1',9)[0],x+1)
        self.assertEqual(self.read('_gs_player2',9)[0],other)
        self.pad(2,low=2,frames=25)
        self.assertLess(self.read('_gs_player2',9)[0],other-1)
        self.pad(2)
        rot=self.read('_gs_player1',50)[0]
        self.pad(1,low=128,frames=30)
        self.assertEqual(self.read('_gs_player1',50)[0],(rot+1)%4)
        self.pad(1)
        self.pad(2,high=128,frames=30)
        board=self.read('_gs_player2',51,240)
        self.assertNotEqual(board,bytes([255])*240)
        self.pad(2,high=128,frames=30)
        self.assertEqual(self.read('_gs_player2',51,240),board)
        self.assertEqual(self.read('_gs_player1',51,240),bytes([255])*240)
        self.pad(2)
        self.assertEqual(self.read('_game_mode'), bytes([2]))
        self.assertEqual(self.emu.read_memory(0x9F29)[0] & 0x7F, 0x31)

    def test_disconnected_and_ai_controllers_cannot_move_boards(self):
        self.install_pads(); self.start(1)
        x=self.read('_gs_player1',9)[0]
        self.pad(1,low=1,frames=30,present=False)
        self.assertEqual(self.read('_gs_player1',9)[0],x)
        self.pad(1); self.press('Escape')
        self.start(2)
        board=self.read('_gs_player1',51,240)
        self.tap(2,high=128)
        self.assertEqual(self.read('_gs_player1',51,240),board)

    def test_start_pauses_both_boards_and_ai_in_every_mode(self):
        self.install_pads()
        for choice in (1, 2, 3, 4):
            with self.subTest(choice=choice):
                self.start(choice)
                self.pad(1, low=16, frames=40)  # Holding Start pauses once.
                self.assertEqual(self.read('_game_paused'), bytes([1]))
                menu.MenuTests.assert_menu_text(self, 29, 'PAUSED', x=2)
                snapshots = [self.read(player, 0, 351) for player in ('_gs_player1', '_gs_player2')]
                self.pad(1)  # Release before the next press.
                self.pad(1, low=1, high=128, frames=10)
                self.pad(2, low=2, high=128, frames=310)  # Cross a full clock wrap.
                for player, snapshot in zip(('_gs_player1', '_gs_player2'), snapshots):
                    self.assertEqual(self.read(player, 0, 351), snapshot)
                self.pad(1); self.pad(2)
                self.tap(2, low=16)  # Either controller can resume.
                self.assertEqual(self.read('_game_paused'), bytes([0]))
                self.assertEqual(self.read('_game_mode'), bytes([choice - 1]))
                if choice == 1:
                    self.assertEqual(self.read('_gs_player1', 9, 8), snapshots[0][9:17])
                self.press('Escape')

    def test_select_opens_quit_dialog_on_both_ports(self):
        self.install_pads(); self.start(3)
        for port in (1, 2):
            snapshot = [self.read(p, 0, 351) for p in ('_gs_player1', '_gs_player2')]
            self.tap(port, low=32)
            self.assertEqual(self.read('_game_quit_open'), bytes([1]))
            menu.MenuTests.assert_menu_text(self, 13, 'QUIT MATCH')
            self.emu.advance(60)  # The match stays frozen behind the dialog.
            self.assertEqual([self.read(p, 0, 351) for p in ('_gs_player1', '_gs_player2')], snapshot)
            self.tap(3 - port, low=32)  # Select on either port cancels.
            self.assertEqual(self.read('_game_quit_open'), bytes([0]))
            self.assertEqual(self.read('_game_paused'), bytes([0]))
            self.assertEqual(self.read('_game_mode'), bytes([2]))
        # Select while paused opens the dialog; cancelling keeps the pause.
        self.tap(2, low=16)
        self.assertEqual(self.read('_game_paused'), bytes([1]))
        self.tap(1, low=32)
        self.assertEqual(self.read('_game_quit_open'), bytes([1]))
        self.pad(1, frames=10)  # A stable release re-arms the same port.
        self.tap(1, low=32)
        self.assertEqual(self.read('_game_quit_open'), bytes([0]))
        self.assertEqual(self.read('_game_paused'), bytes([1]))
        # Confirming from the paused dialog returns to the menu.
        self.tap(2, low=32); self.tap(2, low=2); self.tap(2, high=128)
        self.assertEqual(self.read('_game_paused'), bytes([0]))
        self.assertEqual(self.tile(16, 12), 34)

    def test_simultaneous_start_press_pauses_once(self):
        self.install_pads(); self.start(3)
        self.emu.write_memory(0x600, bytes([255 ^ 16, 255 ^ 16]))
        self.emu.advance(30)
        self.assertEqual(self.read('_game_paused'), bytes([1]))
        self.pad(1); self.pad(2)
        self.tap(1, low=16)
        self.assertEqual(self.read('_game_paused'), bytes([0]))

    def test_pause_preserves_in_progress_row_clear(self):
        self.install_pads(); self.start(3)
        # Start a row wipe, then pause before it can finish.
        self.emu.write_memory(self.symbols['_gs_player1'] + 292, bytes([4]))
        self.emu.write_memory(self.symbols['_gs_player1'] + 301, bytes([1]))
        self.emu.write_memory(self.symbols['_gs_player1'] + 321, bytes([1, 0, 0]))
        self.tap(1, low=16)
        snapshot = self.read('_gs_player1', 0, 351)
        self.assertEqual(snapshot[292], 4)
        self.emu.advance(300)
        self.assertEqual(self.read('_gs_player1', 0, 351), snapshot)
        self.tap(1, low=16)
        self.emu.advance(60)
        self.assertEqual(self.read('_gs_player1', 292), bytes([1]))

    def test_brief_release_does_not_repeat_rotation_or_hard_drop(self):
        self.install_pads(); self.start(3)
        for port, player in ((1, '_gs_player1'), (2, '_gs_player2')):
            rotation = self.read(player, 50)[0]
            self.pad(port, low=128, frames=1)
            self.assertEqual(self.read(player, 50)[0], (rotation + 1) % 4)
            self.pad(port, frames=1)
            self.pad(port, low=128, frames=1)
            self.assertEqual(self.read(player, 50)[0], (rotation + 1) % 4)
            # A different action can still fire while rotation is latched.
            self.pad(port, low=128, high=128, frames=1)
            board = self.read(player, 51, 240)
            self.pad(port, frames=1)
            self.pad(port, high=128, frames=1)
            self.assertEqual(self.read(player, 51, 240), board)
            # A stable release permits the next intentional press promptly.
            self.pad(port, frames=4)
            self.pad(port, high=128, frames=1)
            self.assertNotEqual(self.read(player, 51, 240), board)
            self.pad(port, frames=4)

    def test_start_bounce_does_not_immediately_resume(self):
        self.install_pads(); self.start(3)
        for port in (1, 2):
            self.pad(port, low=16, frames=1)
            self.assertEqual(self.read('_game_paused'), bytes([1]))
            self.pad(port, frames=1)
            self.pad(port, low=16, frames=1)
            self.assertEqual(self.read('_game_paused'), bytes([1]))
            self.pad(port, frames=4)
            self.tap(port, low=16)
            self.assertEqual(self.read('_game_paused'), bytes([0]))
            self.pad(port, frames=4)
