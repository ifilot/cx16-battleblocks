"""Exercise real keyboard input and game-mode transitions in X16 AgentBridge.

Run `make` first; the linker's BBLOCKS.sym supplies current RAM addresses.
Tests use the sibling emulator and ROM, skipping when those are unavailable.
"""

import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest

from PIL import Image, ImageChops


ROOT = Path(__file__).resolve().parents[1]
EMULATOR = ROOT.parent / "x16-emulator/build/x16emu"
ROM = ROOT.parent / "emulator/rom.bin"
API = ROOT.parent / "x16-emulator/api/python"
AVAILABLE = EMULATOR.is_file() and ROM.is_file() and (API / "x16agent.py").is_file()
if AVAILABLE:
    sys.path.insert(0, str(API))
    from x16agent import X16Agent


@unittest.skipUnless(AVAILABLE, "Sibling X16 AgentBridge emulator and ROM required")
class MenuTests(unittest.TestCase):
    def setUp(self):
        labels = ROOT / "src/BBLOCKS.sym"
        if not labels.is_file():
            self.fail("Run make first to create BBLOCKS.PRG and BBLOCKS.sym")
        self.symbols = {}
        for line in labels.read_text().splitlines():
            _, address, name = line.split()
            self.symbols[name.removeprefix(".")] = int(address, 16)
        self.emu = X16Agent(EMULATOR, ROM, fsroot=ROOT / "src",
                            options=["-prg", str(ROOT / "src/BBLOCKS.PRG"), "-run"])
        self.addCleanup(self.emu.close)
        self.emu.advance(180)
        self.press("Space")
        self.emu.advance(30)

    def press(self, key):
        self.emu.request("key", key=key, down=True)
        self.emu.advance(3)
        self.emu.request("key", key=key, down=False)
        self.emu.advance(3)

    def read(self, name, offset=0, length=1):
        return self.emu.read_memory(self.symbols[name] + offset, length)

    def write(self, name, offset, payload):
        self.emu.write_memory(self.symbols[name] + offset, payload)

    def tile(self, x, y):
        return self.emu.read_memory(0x1A000 + (y * 64 + x) * 2, space="vram")[0]

    def assert_cursor(self, row):
        for option in (9, 12, 15, 18, 21):
            self.assertEqual(self.tile(9, option), 34 if option == row else 0)

    def start(self, choice, key="Return"):
        self.press(str(choice))
        self.press(key)
        self.emu.advance(30)
        for _ in range(120):
            if self.emu.read_memory(0x9F29)[0] & 0x30 == 0x30:
                break
            self.emu.advance(1)
        else:
            self.fail("Game display did not finish initializing")
        self.assertEqual(self.read("_game_mode")[0], choice - 1)

    def test_navigation_wraps_and_number_shortcuts_select(self):
        self.assert_cursor(12)
        self.press("Up")
        self.assert_cursor(9)
        self.press("Up")
        self.assert_cursor(21)
        self.press("Down")
        self.assert_cursor(9)
        self.press("2")
        self.assert_cursor(12)
        self.press("3")
        self.assert_cursor(15)
        self.press("4")
        self.assert_cursor(18)
        self.assertEqual(self.tile(7, 23), 12)

    def test_style_option_toggles_without_starting_a_game(self):
        classic = self.emu.read_memory(0x1FA00, 512, space="vram")
        classic_map = self.emu.read_memory(0x18000, 30 * 128, space="vram")
        field_tiles = self.emu.read_memory(2 * 256, 512, space="vram")
        self.press("5")
        self.assert_cursor(21)
        self.press("Return")
        self.assertEqual(self.read("_screen_style"), bytes([1]))
        self.assert_cursor(21)
        self.assertEqual(self.emu.read_memory(0x18000, 2, space="vram"), bytes([112, 1]))
        self.assertNotEqual(self.emu.read_memory(0x1FA00, 512, space="vram"), classic)
        self.assertEqual(self.emu.read_memory(2 * 256, 512, space="vram"), field_tiles)
        self.press("Space")
        self.assertEqual(self.read("_screen_style"), bytes([0]))
        self.assertEqual(self.emu.read_memory(0x1FA00, 512, space="vram"), classic)
        self.assertEqual(self.emu.read_memory(0x18000, 30 * 128, space="vram"), classic_map)

    def test_style_survives_game_and_splash_transitions(self):
        classic = self.emu.read_memory(0x1FA00, 512, space="vram")
        self.press("T")
        cyberpunk = self.emu.read_memory(0x1FA00, 512, space="vram")
        self.start(3)
        self.assertEqual(self.emu.read_memory(0x1FA00, 512, space="vram"), cyberpunk)
        self.assertEqual(self.read("_gs_player1", 292), bytes([1]))
        self.assertEqual(self.emu.read_memory(368 * 256, 4096, space="vram"),
                         (ROOT / "src/CYBER.DAT").read_bytes())
        self.press("Escape")
        self.emu.advance(30)
        self.assertEqual(self.read("_screen_style"), bytes([1]))
        self.press("Escape")
        self.emu.advance(90)
        self.assertEqual(self.read("_screen_style"), bytes([1]))
        self.press("Space")
        self.emu.advance(30)
        self.assertEqual(self.emu.read_memory(0x1FA00, 512, space="vram"), cyberpunk)
        self.press("T")
        self.assertEqual(self.emu.read_memory(0x1FA00, 512, space="vram"), classic)
        self.start(1)
        self.assertEqual(self.emu.read_memory(0x1FA00, 512, space="vram"), classic)

    def test_enter_starts_solo_with_gravity_and_no_opponent(self):
        self.start(1)
        self.assertEqual(self.read("_gs_player2", 292)[0], 3)
        self.assertEqual(self.read("_gs_player2", 51, 240), bytes([255]) * 240)

        y = self.read("_gs_player1", 10)[0]
        self.emu.advance(120)
        self.assertLess(self.read("_gs_player1", 10)[0], y)
        self.assertEqual(self.read("_gs_player2", 51, 240), bytes([255]) * 240)

    def test_approved_banners_follow_modes_and_survive_style_and_title_changes(self):
        sys.path.insert(0, str(ROOT / 'assets/scripts'))
        from bitmap_font import FONT
        from PIL import ImageDraw
        designs = ROOT / 'tests/fixtures/banners'
        original_tiles = (ROOT / 'src/TILES.DAT').read_bytes()
        modes = ((1, ((False, 1),)), (2, ((False, 1), (True, 1))),
                 (3, ((False, 1), (False, 2))), (4, ((True, 1), (True, 2))))
        with TemporaryDirectory(prefix='battleblocks-banners-') as directory:
            for style in ('light', 'cyberpunk'):
                if style == 'cyberpunk':
                    # Loading the splash replaces artwork; returning must reload banners.
                    self.press('Escape')
                    self.emu.advance(90)
                    self.press('Space')
                    self.emu.advance(30)
                    self.press('T')
                palette = json.loads((ROOT / 'assets/palette' / (style + '.json')).read_text())
                for choice, players in modes:
                    self.start(choice)
                    path = Path(directory) / 'actual.png'
                    self.emu.screenshot(path)
                    actual = Image.open(path).convert('RGB')
                    for player, (computer, number) in enumerate(players):
                        role = 'computer' if computer else 'player'
                        expected = Image.open(designs / f'{style}-{role}.png').convert('RGB')
                        if number == 2:
                            # Change only the approved banner's last numeral.
                            x = 160 if computer else 136
                            draw = ImageDraw.Draw(expected)
                            draw.rectangle((x, 7, x + 9, 20), fill=tuple(palette[30]))
                            for row, bits in enumerate(FONT['2']):
                                for col, bit in enumerate(bits):
                                    if bit == '1':
                                        draw.rectangle((x + col * 2, 7 + row * 2,
                                                        x + col * 2 + 1, 8 + row * 2),
                                                       fill=tuple(palette[11]))
                        x = 16 + player * 320
                        rendered = actual.crop((x, 0, x + 274, 28))
                        self.assertIsNone(ImageChops.difference(rendered, expected).getbbox(),
                                          f'{style}, mode {choice}, player {player + 1}')
                    if choice == 1:
                        self.assertEqual(self.tile(21, 0), 0)
                    # Banner atlases must preserve every legacy game tile,
                    # including all seven composite next-piece previews.
                    for offset in range(0, 128 * 256, 8192):
                        self.assertEqual(self.emu.read_memory(offset, 8192, space='vram'),
                                         original_tiles[offset:offset + 8192])
                    self.press('Escape')
                    self.emu.advance(30)

    def test_space_starts_computer_mode_and_ai_plays(self):
        self.start(2, "Space")
        self.emu.advance(180)
        self.assertNotEqual(self.read("_gs_player2", 51, 240), bytes([255]) * 240)

    def test_demo_both_computers_play_and_escape_returns(self):
        self.start(4)
        self.emu.advance(360)
        for player in ("_gs_player1", "_gs_player2"):
            self.assertNotEqual(self.read(player, 51, 240), bytes([255]) * 240)
            self.assertEqual(self.read(player, 292)[0], 1)
        self.press("Escape")
        self.emu.advance(30)
        self.assert_cursor(18)

    def prepare_clear(self, player, rows):
        field = bytearray([255] * 240)
        for row in rows:
            field[row * 10:row * 10 + 10] = bytes([2]) * 10
            field[row * 10 + 4] = 255
        # Markers above the cleared rows, including the hidden spawn area.
        field[(max(rows) + 1) * 10] = 3
        if max(rows) < 19:
            field[19 * 10 + 1] = 5
        field[20 * 10 + 2] = 4
        self.write(player, 51, field)
        coords = bytes(value for row in range(min(rows), min(rows) + 4) for value in (4, row))
        self.write(player, 9, coords)
        self.write(player, 25, coords)
        self.write(player, 49, bytes([0, 1]))  # vertical I
        xoffset = self.read(player, 291)[0]
        for row in range(20):
            for col in range(10):
                tile = 0 if field[row * 10 + col] == 255 else 16 + field[row * 10 + col]
                address = 0x1A000 + ((27 - row) * 64 + xoffset + col) * 2
                self.emu.write_memory(address, bytes([tile]), space="vram")
        return field

    def test_four_line_clear_animates_then_compacts_hidden_rows_and_scores(self):
        self.start(3)
        self.prepare_clear("_gs_player1", range(4))
        opponent = self.read("_gs_player2", 51, 240)
        sprites = self.emu.read_memory(0x1FC00, 1024, space="vram")
        self.emu.request("key", key="Space", down=True)
        self.emu.advance(5)
        # The wipe stays inside the row: an empty prefix, then intact blocks.
        for row in (3,):
            tiles = [self.tile(1 + col, 27 - row) for col in range(10)]
            self.assertEqual(tiles[0], 0)
            self.assertNotEqual(tiles[-1], 0)
            first_block = next(col for col, tile in enumerate(tiles) if tile)
            self.assertTrue(all(tiles[first_block:]))
            self.assertEqual(self.tile(0, 27 - row), 11)
            self.assertEqual(self.hud_pixels(176, (27 - row) * 16, 2, 1), [[16, 16]])
        for row in range(3):
            self.assertTrue(all(self.tile(1 + col, 27 - row) for col in range(10)))
        self.assertEqual(self.emu.read_memory(0x1FC00, 1024, space="vram"), sprites)
        # Rows have not collapsed while the wipe is in progress.
        self.assertEqual(self.read("_gs_player1", 51 + 40)[0], 3)
        self.emu.request("key", key="Space", down=False)
        locked_piece = self.read("_gs_player1", 9, 8)
        self.press("Right")
        self.assertEqual(self.read("_gs_player1", 9, 8), locked_piece)
        self.emu.advance(20)
        self.assertEqual(self.read("_gs_player1", 292)[0], 4)
        self.assertTrue(all(self.tile(1 + col, 24) == 0 for col in range(10)))
        self.assertTrue(all(self.tile(1 + col, 27) for col in range(10)))
        self.assertEqual(self.read("_gs_player1", 293, 6), bytes(6))
        self.emu.advance(100)
        self.assertEqual(int.from_bytes(self.read("_gs_player1", 293, 4), "little"), 1200)
        self.assertEqual(int.from_bytes(self.read("_gs_player1", 297, 2), "little"), 4)
        self.assertEqual(self.read("_gs_player1", 51)[0], 3)
        self.assertEqual(self.read("_gs_player1", 51 + 15 * 10 + 1)[0], 5)
        self.assertEqual(self.read("_gs_player1", 51 + 16 * 10 + 2)[0], 4)
        self.assertEqual(self.read("_gs_player1", 51 + 20 * 10, 40), bytes([255]) * 40)
        self.assertEqual(self.read("_gs_player2", 51, 240), opponent)
        self.assertEqual(self.emu.read_memory(0x1FC00, 1024, space="vram"), sprites)

    def test_other_player_can_move_and_drop_during_clear(self):
        self.start(3)
        self.prepare_clear("_gs_player1", range(4))
        self.press("Space")
        self.assertEqual(self.read("_gs_player1", 292)[0], 4)
        x = self.read("_gs_player2", 9)[0]
        self.press("D")
        self.assertEqual(self.read("_gs_player2", 9)[0], x + 1)
        self.press("Q")
        self.assertNotEqual(self.read("_gs_player2", 51, 240), bytes([255]) * 240)
        self.assertEqual(self.read("_gs_player1", 292)[0], 4)
        self.press("Escape")
        self.emu.advance(30)
        self.assert_cursor(15)
        self.press("Return")
        self.emu.advance(30)
        self.assertEqual(self.read("_gs_player1", 292)[0], 1)
        self.assertEqual(self.read("_gs_player1", 51, 240), bytes([255]) * 240)

    def test_both_boards_clear_independently_and_finish(self):
        self.start(3)
        self.prepare_clear("_gs_player1", range(4))
        self.prepare_clear("_gs_player2", range(4))
        # Keep headroom for the new attacks exchanged by these clears.
        for player in ("_gs_player1", "_gs_player2"):
            self.write(player, 51 + 19 * 10 + 1, bytes([255]))
            self.write(player, 51 + 20 * 10 + 2, bytes([255]))
        self.press("Space")
        self.emu.advance(6)
        self.press("Q")
        self.assertEqual(self.read("_gs_player1", 292)[0], 4)
        self.assertEqual(self.read("_gs_player2", 292)[0], 4)
        self.assertGreater(self.read("_gs_player1", 321)[0], self.read("_gs_player2", 321)[0])
        self.emu.advance(110)
        for player in ("_gs_player1", "_gs_player2"):
            self.assertEqual(self.read(player, 292)[0], 1)
            self.assertEqual(int.from_bytes(self.read(player, 293, 4), "little"), 1200)
            self.assertEqual(int.from_bytes(self.read(player, 297, 2), "little"), 4)

    def test_computer_keeps_playing_during_human_clear(self):
        self.start(2)
        self.prepare_clear("_gs_player1", range(4))
        self.press("Space")
        opponent = self.read("_gs_player2", 51, 240)
        self.emu.advance(45)
        self.assertEqual(self.read("_gs_player1", 292)[0], 4)
        self.assertNotEqual(self.read("_gs_player2", 51, 240), opponent)

    def test_right_board_clear_and_top_visible_row(self):
        self.start(3)
        self.prepare_clear("_gs_player2", [19])
        self.press("Q")
        self.emu.advance(40)
        self.assertEqual(int.from_bytes(self.read("_gs_player2", 293, 4), "little"), 40)
        self.assertEqual(int.from_bytes(self.read("_gs_player2", 297, 2), "little"), 1)
        self.assertEqual(self.read("_gs_player2", 51 + 19 * 10)[0], 3)
        self.assertEqual(self.read("_gs_player2", 51 + 19 * 10 + 2)[0], 4)

    def assert_garbage(self, player, count):
        field = self.read(player, 51, 240)
        holes = []
        for row in range(count):
            cells = field[row * 10:row * 10 + 10]
            self.assertEqual(cells.count(255), 1)
            holes.append(cells.index(255))
            # Independent colour choices may produce a row of one shade.
            self.assertLessEqual(set(cells) - {255}, {112, 113})
        return holes

    def test_attack_strength_and_delivery_at_next_piece(self):
        self.start(3)
        for cleared, sent in ((1, 0), (2, 1), (3, 2), (4, 4)):
            with self.subTest(cleared=cleared):
                if cleared > 1:
                    self.press("Escape")
                    self.emu.advance(30)
                    self.press("Return")
                    self.emu.advance(30)
                self.prepare_clear("_gs_player1", range(cleared))
                opponent = self.read("_gs_player2", 51, 240)
                self.press("Space")
                self.emu.advance(105)
                self.assertEqual(self.read("_gs_player2", 323)[0], sent)
                self.assertEqual(self.read("_gs_player2", 51, 240), opponent)
                # Keep the active opponent piece stationary until it locks.
                self.press("Q")
                self.assertEqual(self.read("_gs_player2", 323)[0], 0)
                if sent:
                    self.assert_garbage("_gs_player2", sent)
                self.assertEqual(self.read("_gs_player2", 292)[0], 1)

    def test_attacks_accumulate_and_restart_clears_queue(self):
        self.start(3)
        for _ in range(2):
            self.prepare_clear("_gs_player1", range(4))
            self.press("Space")
            self.emu.advance(105)
        self.assertEqual(self.read("_gs_player2", 323)[0], 8)
        self.press("Q")
        holes = self.assert_garbage("_gs_player2", 8)
        self.assertGreater(len(set(holes)), 1)
        self.write("_gs_player1", 323, bytes([7]))
        self.press("Escape")
        self.emu.advance(30)
        self.press("Return")
        self.emu.advance(30)
        for player in ("_gs_player1", "_gs_player2"):
            self.assertEqual(self.read(player, 323)[0], 0)

    def test_random_garbage_can_be_cleared_normally(self):
        self.start(3)
        self.write("_gs_player2", 323, bytes([1]))
        self.press("Q")
        hole = self.assert_garbage("_gs_player2", 1)[0]
        # Isolate the generated row, leaving room for an I to fill its gap.
        self.write("_gs_player2", 51 + 10, bytes([255]) * 230)
        coords = bytes(value for row in range(4) for value in (hole, row))
        self.write("_gs_player2", 9, coords)
        self.write("_gs_player2", 25, coords)
        self.write("_gs_player2", 49, bytes([0, 1]))
        self.press("Q")
        self.emu.advance(40)
        self.assertEqual(self.read("_gs_player2", 297, 2), (1).to_bytes(2, "little"))
        self.assertEqual(self.read("_gs_player2", 293, 4), (40).to_bytes(4, "little"))
        self.assertFalse(any(cell in (112, 113) for cell in self.read("_gs_player2", 51, 240)))
        self.assertEqual(self.read("_gs_player1", 323)[0], 0)

    def test_garbage_pushes_existing_stack_up_and_causes_top_out(self):
        self.start(3)
        self.write("_gs_player2", 51 + 19 * 10, bytes([2]))
        self.write("_gs_player2", 323, bytes([4]))
        self.press("Q")
        self.emu.advance(40)
        self.assertEqual(self.read("_gs_player2", 292)[0], 2)
        self.assertEqual(self.read("_gs_player2", 51 + 23 * 10)[0], 2)
        self.assertEqual(self.tile(15, 14), 96 + ord("G") - ord("A"))

    def test_solo_clear_sends_no_garbage(self):
        self.start(1)
        self.prepare_clear("_gs_player1", range(4))
        self.press("Space")
        self.emu.advance(105)
        self.assertEqual(self.read("_gs_player2", 323)[0], 0)
        self.assertEqual(self.read("_gs_player2", 51, 240), bytes([255]) * 240)

    def test_local_player_two_has_manual_controls_without_ai(self):
        self.start(3)
        self.emu.advance(180)
        self.assertEqual(self.read("_gs_player2", 51, 240), bytes([255]) * 240)
        x = self.read("_gs_player2", 9)[0]
        p1x = self.read("_gs_player1", 9)[0]
        self.press("D")
        self.assertEqual(self.read("_gs_player2", 9)[0], x + 1)
        self.assertEqual(self.read("_gs_player1", 9)[0], p1x)
        self.press("A")
        self.assertEqual(self.read("_gs_player2", 9)[0], x)
        rotation = self.read("_gs_player2", 50)[0]
        self.press("W")
        self.assertEqual(self.read("_gs_player2", 50)[0], (rotation + 1) % 4)
        self.press("Q")
        self.assertNotEqual(self.read("_gs_player2", 51, 240), bytes([255]) * 240)
        self.assertEqual(self.read("_gs_player1", 51, 240), bytes([255]) * 240)
        self.press("Space")
        self.assertNotEqual(self.read("_gs_player1", 51, 240), bytes([255]) * 240)

    def test_escape_returns_to_menu_and_restarting_resets_stats(self):
        self.start(3)
        self.write("_gs_player1", 293, (1234).to_bytes(4, "little"))
        self.write("_gs_player1", 297, (17).to_bytes(2, "little"))
        self.press("Escape")
        self.emu.advance(30)
        self.assert_cursor(15)
        self.press("Return")
        self.emu.advance(30)
        self.assertEqual(self.read("_gs_player1", 293, 6), bytes(6))
        self.assertEqual(self.read("_gs_player2", 293, 6), bytes(6))

    def test_escape_from_menu_restores_selected_splash(self):
        self.press("Escape")
        self.emu.advance(60)
        with TemporaryDirectory(prefix="battleblocks-menu-") as directory:
            capture = Path(directory) / "splash.png"
            self.assertTrue(self.emu.screenshot(capture)["captured"])
            with Image.open(capture) as actual, Image.open(ROOT / "assets/png/titlescreen.png") as source:
                self.assertIsNone(ImageChops.difference(source.convert("RGB"), actual).getbbox())
        self.press("Return")
        self.emu.advance(30)
        self.assert_cursor(12)

    def test_escape_skips_game_over_animation_immediately(self):
        self.start(3)
        self.write("_gs_player2", 292, bytes([2]))
        self.write("_gs_player2", 324, bytes([0, 0]))
        self.emu.advance(1)
        self.press("Escape")
        self.emu.advance(5)
        self.assert_cursor(15)
        self.assertLess(self.read("_gs_player2", 324)[0], 20)

    def test_blocked_ai_moves_eventually_lock_instead_of_stalling(self):
        self.start(2)
        field = bytearray([255] * 240)
        field[10 * 10 + 4] = 2  # Blocks the planned left shift of an O.
        self.write("_gs_player2", 51, field)
        self.write("_gs_player2", 1, bytes([22]) * 8)
        self.write("_gs_player2", 9, bytes((5, 10, 6, 10, 5, 11, 6, 11)))
        self.write("_gs_player2", 17, bytes([22]) * 8)
        self.write("_gs_player2", 25, bytes((5, 0, 6, 0, 5, 1, 6, 1)))
        self.write("_gs_player2", 49, bytes([1, 0]))
        self.write("_pfeval", 192, bytes([4, 0, 0, 4]))  # Finished plan: rotation 0, column 4.
        self.write("_pfeval", 233, bytes([0]))  # Reset blocked-movement count.
        self.write("_pfeval", 355, bytes([2]))  # Both search stages are finished.
        self.emu.advance(25)
        self.assertEqual(self.read("_gs_player2", 51 + 5)[0], 1)
        self.assertEqual(self.read("_gs_player2", 51 + 6)[0], 1)

    def hud_pixels(self, x, y, width, height):
        tiles = {}
        result = []
        for py in range(y, y + height):
            row = []
            for px in range(x, x + width):
                cell = (px // 16, py // 16)
                if cell not in tiles:
                    entry = self.emu.read_memory(0x1A000 + (cell[1] * 64 + cell[0]) * 2, 2, space="vram")
                    tile = entry[0] | ((entry[1] & 3) << 8)
                    tiles[cell] = self.emu.read_memory(tile * 256, 256, space="vram")
                row.append(tiles[cell][(py % 16) * 16 + px % 16])
            result.append(row)
        return result

    def assert_hud_number(self, player, field, value):
        sys.path.insert(0, str(ROOT / "assets/scripts"))
        from bitmap_font import FONT
        from build_hud import FIELDS as fields
        x, y, width, height, scale, background = fields[field]
        expected = [[background] * width for _ in range(height)]
        text = str(value)
        start = width - (len(text) * 6 - 1) * scale
        for i, char in enumerate(text):
            for row, bits in enumerate(FONT[char]):
                for col, bit in enumerate(bits):
                    if bit == '1':
                        for dy in range(scale):
                            for dx in range(scale):
                                expected[row * scale + dy][start + i * 6 * scale + col * scale + dx] = 11
        self.assertEqual(self.hud_pixels(x + player * 320, y, width, height), expected)

    def test_split_cards_render_maximum_values_after_clear(self):
        self.start(3)
        self.prepare_clear("_gs_player1", range(4))
        self.write("_gs_player1", 51 + 19 * 10 + 1, bytes([255]))
        self.write("_gs_player1", 51 + 20 * 10 + 2, bytes([255]))
        self.write("_gs_player1", 293, (4294967295 - 1200 * 6554).to_bytes(4, "little") + (65531).to_bytes(2, "little"))
        self.write("_gs_player1", 348, (6553).to_bytes(2, "little"))
        index = self.read("_gs_player1", 40)[0]
        piece = self.read("_gs_player1", 33 + index)[0]
        self.write("_gs_player1", 326 + piece * 2, (65534).to_bytes(2, "little"))
        self.write("_gs_player1", 340 + 6, (65534).to_bytes(2, "little"))
        self.press("Space")
        self.emu.advance(110)
        for field, value in ((0, 4294967295), (1, 6553), (2, 65535), (3 + piece, 65535), (13, 65535)):
            self.assert_hud_number(0, field, value)
        self.assertEqual(self.read("_gs_player1", 291)[0], 1)
        self.assertEqual(self.read("_gs_player2", 291)[0], 21)
        # Composite HUD tiles preserve the adjacent playfield rim.
        self.assertEqual(self.hud_pixels(176, 400, 2, 1), [[16, 16]])

    def statistics(self, player):
        data = self.read(player, 326, 22)
        return [int.from_bytes(data[i:i + 2], "little") for i in range(0, 22, 2)]

    def test_statistics_count_spawns_and_each_clear_type_and_reset(self):
        self.start(3)
        for player, offset in (("_gs_player1", 0), ("_gs_player2", 320)):
            counts = self.statistics(player)
            self.assertEqual(sum(counts[:7]), 1)
            self.assertEqual(counts[self.read(player, 49)[0]], 1)
            self.assertEqual(counts[7:], [0] * 4)
            for piece in range(7):
                pixels = self.hud_pixels(194 + offset, 262 + piece * 16, 32, 16)
                face = (ROOT / "src/TILES.DAT").read_bytes()[(16 + piece) * 256 + 136]
                self.assertIn(face, [pixel for row in pixels for pixel in row])
                self.assert_hud_number(offset // 320, 3 + piece, counts[piece])
        for rows in range(1, 5):
            self.prepare_clear("_gs_player1", range(rows))
            self.write("_gs_player1", 51 + 19 * 10 + 1, bytes([255]))
            self.write("_gs_player1", 51 + 20 * 10 + 2, bytes([255]))
            self.press("Space")
            self.assertEqual(self.statistics("_gs_player1")[6 + rows], 0)
            self.emu.advance(rows * 22 + 12)
            self.assertEqual(self.statistics("_gs_player1")[7:], [1] * rows + [0] * (4 - rows))
            self.assertEqual(sum(self.statistics("_gs_player1")[:7]), rows + 1)
            self.assertEqual(self.statistics("_gs_player2")[7:], [0] * 4)
            self.assert_hud_number(0, 9 + rows, 1)
        self.press("Escape")
        self.press("Return")
        self.emu.advance(30)
        for player in ("_gs_player1", "_gs_player2"):
            counts = self.statistics(player)
            self.assertEqual(sum(counts[:7]), 1)
            self.assertEqual(counts[7:], [0] * 4)

    def test_statistics_show_five_digit_counts_and_solo_has_no_opponent_stats(self):
        self.start(1)
        self.assertEqual(self.statistics("_gs_player2"), [0] * 11)
        self.assertEqual(self.tile(34, 16), 0)
        index = self.read("_gs_player1", 40)[0]
        piece = self.read("_gs_player1", 33 + index)[0]
        self.write("_gs_player1", 326 + piece * 2, (12344).to_bytes(2, "little"))
        self.press("Space")
        self.assertEqual(self.statistics("_gs_player1")[piece], 12345)
        self.assert_hud_number(0, 3 + piece, 12345)

    def level(self, player):
        return int.from_bytes(self.read(player, 348, 2), "little")

    def clean_clear_fixture(self, player, count):
        self.prepare_clear(player, range(count))
        self.write(player, 51 + 19 * 10 + 1, bytes([255]))
        self.write(player, 51 + 20 * 10 + 2, bytes([255]))

    def test_levels_advance_at_ten_lines_after_wipe_and_reset(self):
        self.start(3)
        self.write("_gs_player1", 297, (9).to_bytes(2, "little"))
        self.clean_clear_fixture("_gs_player1", 1)
        self.press("Space")
        self.assertEqual(self.level("_gs_player1"), 0)
        self.emu.advance(40)
        self.assertEqual(self.level("_gs_player1"), 1)
        self.assertEqual(int.from_bytes(self.read("_gs_player1", 293, 4), "little"), 40)
        self.assert_hud_number(0, 1, 1)
        self.assertEqual(self.level("_gs_player2"), 0)
        self.write("_gs_player1", 297, (19).to_bytes(2, "little"))
        self.write("_gs_player1", 293, bytes(4))
        self.clean_clear_fixture("_gs_player1", 4)
        self.press("Space")
        self.emu.advance(110)
        self.assertEqual(self.level("_gs_player1"), 2)
        self.assertEqual(int.from_bytes(self.read("_gs_player1", 297, 2), "little"), 23)
        self.assertEqual(int.from_bytes(self.read("_gs_player1", 293, 4), "little"), 2400)
        self.assert_hud_number(0, 1, 2)
        self.assertEqual(self.read("_gs_player2", 323)[0], 4)  # Level leaves attack sizes alone.
        self.assertEqual(self.level("_gs_player2"), 0)
        self.press("Escape")
        self.start(1)
        self.assertEqual(self.level("_gs_player1"), 0)
        self.assertEqual(self.level("_gs_player2"), 0)
        self.assert_hud_number(0, 1, 0)

    def test_all_clear_sizes_use_level_multiplier(self):
        self.start(3)
        for count, base in enumerate((40, 100, 300, 1200), 1):
            self.write("_gs_player1", 297, (20).to_bytes(2, "little"))
            self.write("_gs_player1", 348, (2).to_bytes(2, "little"))
            self.write("_gs_player1", 293, bytes(4))
            self.clean_clear_fixture("_gs_player1", count)
            self.press("Space")
            self.emu.advance(count * 22 + 18)
            self.assertEqual(int.from_bytes(self.read("_gs_player1", 293, 4), "little"), base * 3)
            self.assertEqual(self.level("_gs_player1"), 2)
            self.assert_hud_number(0, 0, base * 3)
            self.assert_hud_number(0, 1, 2)

    def test_gravity_uses_each_players_level_and_caps_high_levels(self):
        self.start(3)
        self.press("Down")  # Give the slow board a fresh gravity interval.
        tick = self.read("_gs_player1", 350)
        self.write("_gs_player2", 350, tick)
        self.write("_gs_player2", 348, (13).to_bytes(2, "little"))
        slow = self.read("_gs_player1", 10)[0]
        fast = self.read("_gs_player2", 10)[0]
        self.emu.advance(12)
        self.assertEqual(self.read("_gs_player1", 10)[0], slow)
        self.assertGreaterEqual(fast - self.read("_gs_player2", 10)[0], 4)
        # A large level must use the capped speed, never index beyond the table.
        self.write("_gs_player2", 348, (6553).to_bytes(2, "little"))
        fast = self.read("_gs_player2", 10)[0]
        self.emu.advance(12)
        self.assertGreaterEqual(fast - self.read("_gs_player2", 10)[0], 4)
        self.assertEqual(self.read("_gs_player1", 10)[0], slow)

    def test_level_and_score_totals_saturate_instead_of_wrapping(self):
        self.start(3)
        self.write("_gs_player1", 297, (65535).to_bytes(2, "little"))
        self.write("_gs_player1", 348, (6553).to_bytes(2, "little"))
        self.write("_gs_player1", 293, (4294967295).to_bytes(4, "little"))
        self.clean_clear_fixture("_gs_player1", 1)
        self.press("Space")
        self.emu.advance(40)
        self.assertEqual(self.level("_gs_player1"), 6553)
        self.assertEqual(int.from_bytes(self.read("_gs_player1", 297, 2), "little"), 65535)
        self.assertEqual(int.from_bytes(self.read("_gs_player1", 293, 4), "little"), 4294967295)
        self.assert_hud_number(0, 1, 6553)

    def test_computers_continue_playing_at_fastest_gravity(self):
        self.start(4)
        for player in ("_gs_player1", "_gs_player2"):
            self.write(player, 297, (130).to_bytes(2, "little"))
            self.write(player, 348, (13).to_bytes(2, "little"))
        self.emu.advance(180)
        for player in ("_gs_player1", "_gs_player2"):
            self.assertGreater(sum(self.statistics(player)[:7]), 2)
        self.press("Escape")
        self.emu.advance(30)
        self.assert_cursor(18)

    def test_ai_preview_tracks_bag_rollover(self):
        self.start(2)
        self.write("_gs_player2", 33 + 6, bytes([4, 6]))  # Last current-bag piece, index 6.
        self.write("_gs_player2", 41, bytes([6, 5, 4, 3, 2, 1, 0, 0]))
        self.write("_pfeval", 194, bytes([255]))  # Lock the active piece now.
        self.write("_pfeval", 355, bytes([2]))
        # Observe the completed spawn, rather than a point midway through HUD/audio updates.
        for _ in range(12):
            self.emu.advance(1)
            if self.read("_pfeval", 356)[0] == 6:
                break
        self.assertEqual(self.read("_gs_player2", 49)[0], 4)
        self.assertEqual(self.read("_gs_player2", 33, 7), bytes([6, 5, 4, 3, 2, 1, 0]))
        self.assertEqual(self.read("_gs_player2", 40)[0], 0)
        self.assertEqual(self.read("_pfeval", 356)[0], 6)

    def test_finished_match_returns_to_menu(self):
        self.start(3)
        # Simulate a completed match at the state boundary, avoiding hundreds
        # of artificial drops just to reach the result-screen branch.
        self.write("_gs_player2", 292, bytes([2]))
        self.emu.advance(4)
        self.assertEqual(self.tile(15, 14), 96 + ord("G") - ord("A"))
        self.press("Return")
        self.emu.advance(30)
        self.assert_cursor(15)


if __name__ == "__main__":
    unittest.main()
