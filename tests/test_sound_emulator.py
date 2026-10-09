"""Verify bundled ZSM audio on the running game, through real PSG registers."""
import re
import shutil
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

import test_menu_emulator as menu_tests

AVAILABLE = menu_tests.AVAILABLE
ROOT = menu_tests.ROOT

PSG = 0x1F9C0


def music_states(data):
    """Decode PSG-only streams into snapshots, including music attenuation 4."""
    state = bytearray(40)
    states = []
    position = 16
    loop_tick = None
    loop_offset = int.from_bytes(data[3:6], 'little')
    while position < len(data):
        if position == loop_offset:
            loop_tick = len(states)
        command = data[position]
        position += 1
        if command < 0x40:
            value = data[position]
            position += 1
            if command < 40:
                if command & 3 == 2:
                    value = (value & 0xC0) | max(0, (value & 63) - 4)
                state[command] = value
        elif command == 0x40:
            size = data[position] & 63
            position += 1 + size
        elif command < 0x80:
            raise AssertionError('Unexpected FM commands in PSG assets')
        elif command == 0x80:
            break
        else:
            states.extend([bytes(state)] * (command & 127))
    return states, loop_tick


@unittest.skipUnless(AVAILABLE, 'Sibling X16 AgentBridge emulator and ROM required')
class SoundTests(unittest.TestCase):
    def setUp(self):
        self.game = menu_tests.MenuTests()
        self.game.setUp()
        self.addCleanup(self.game.doCleanups)
        self.emu = self.game.emu
        self.assertEqual(self.game.read('_sound_ready'), bytes([1]))

    def test_missing_audio_disables_playback_and_game_still_runs(self):
        with TemporaryDirectory(prefix='battleblocks-no-audio-') as path:
            path = Path(path)
            for name in ('BBLOCKS.PRG', 'TILES.DAT', 'CYBER.DAT', 'HUD.DAT',
                         'TITLE.DAT', 'TITLEMAP.DAT', 'TITLEPAL.DAT', 'FONTMAP.DAT',
                         'NAME-L.DAT', 'NAME-LX.DAT', 'NAME-C.DAT', 'NAME-CX.DAT'):
                shutil.copyfile(ROOT / 'src' / name, path / name)
            with menu_tests.X16Agent(menu_tests.EMULATOR, menu_tests.ROM, fsroot=path,
                                    options=['-prg', str(path / 'BBLOCKS.PRG'), '-run']) as silent:
                silent.advance(180)
                self.assertEqual(silent.read_memory(self.game.symbols['_sound_ready']), bytes([0]))
                for key in ('Space', '1', 'Return'):
                    silent.request('key', key=key, down=True)
                    silent.advance(3)
                    silent.request('key', key=key, down=False)
                    silent.advance(30)
                silent.advance(60)
                self.assertEqual(silent.read_memory(self.game.symbols['_game_mode']), bytes([0]))
                self.assertEqual(silent.read_memory(self.game.symbols['_gs_player1'] + 292), bytes([1]))

    def assert_music_progresses(self, name, frames=60):
        data = (ROOT / 'assets/sound' / name).read_bytes()
        mask = int.from_bytes(data[10:12], 'little')
        registers = [r for r in range(40) if mask & (1 << (r // 4))]
        reference, _ = music_states(data)
        reference = [bytes(state[r] for r in registers) for state in reference]
        # Sample midway between frame interrupts to avoid partially written ticks.
        matched = None
        samples = set()
        for _ in range(frames):
            self.emu.advance(1)
            self.emu.advance(30000, unit='cycles')
            state = self.emu.read_memory(PSG, 40, space='vram')
            actual = bytes(state[r] for r in registers)
            samples.add(actual)
            # Sustained notes can match several adjacent ticks. Keep every
            # possible alignment until a later note distinguishes them.
            candidates = (range(min(180, len(reference))) if matched is None else
                          {i + step for i in matched for step in range(4)})
            found = {i for i in candidates if i < len(reference) and reference[i] == actual}
            self.assertTrue(found, f'{name}: PSG snapshot deviated near ticks {matched}')
            matched = found
        self.assertGreater(len(samples), 3, f'{name}: music did not advance')

    def test_all_audio_files_loaded_across_ram_banks(self):
        header = (ROOT / 'assets/sound/music.h').read_text()
        for name, first in re.findall(r'^#define MUSIC_(\w+)_BANK (\d+)', header, re.M):
            if name in ('FIRST', 'LAST'):
                continue
            data = (ROOT / 'assets/sound' / (name + '.ZSM')).read_bytes()
            for offset in range(0, len(data), 8192):
                chunk = data[offset:offset + 8192]
                self.assertEqual(self.emu.read_memory(0, len(chunk), space='banked_ram',
                                                      bank=int(first) + offset // 8192), chunk)
        data = (ROOT / 'assets/sound/SFX.BIN').read_bytes()
        self.assertEqual(self.emu.read_memory(0, len(data), space='banked_ram', bank=2), data)

    def test_title_and_every_battle_variant_play_in_order(self):
        self.assert_music_progresses('TITLE.ZSM')
        for name, bank in (('BATTLE.ZSM', 14), ('BATTLE2.ZSM', 18),
                           ('BATTLE3.ZSM', 21), ('BATTLE4.ZSM', 26), ('BATTLE.ZSM', 14)):
            self.game.start(1)
            self.assertEqual(self.game.read('_sound_current_music'), bytes([bank]))
            self.assert_music_progresses(name)
            self.game.press('Escape')
            self.emu.advance(30)
            self.assertEqual(self.game.read('_sound_current_music'), bytes([10]))

    def test_title_loop_and_cross_bank_playback(self):
        reference, loop_tick = music_states((ROOT / 'assets/sound/TITLE.ZSM').read_bytes())
        states = set(reference)
        loop_states = set(reference[loop_tick:loop_tick + 300])
        for _ in range(7):
            self.emu.advance(600)  # More than the 60-second track, including its loop.
            self.emu.advance(30000, unit='cycles')
            self.assertIn(self.emu.read_memory(PSG, 40, space='vram'), states)
        after_loop = {self.emu.read_memory(PSG, 40, space='vram')}
        for _ in range(120):
            self.emu.advance(1)
            self.emu.advance(30000, unit='cycles')
            after_loop.add(self.emu.read_memory(PSG, 40, space='vram'))
        self.assertGreater(len(after_loop), 3)
        self.assertTrue(loop_states)  # Header actually defines an intro/loop point.

    def test_both_players_harddrop_effects_overlap_and_release(self):
        self.game.start(3)
        for key in ('Space', 'Q'):
            self.emu.request('key', key=key, down=True)
        self.emu.advance(3)
        registers = self.emu.read_memory(PSG, 64, space='vram')
        self.assertGreater(registers[12 * 4 + 2] & 63, 0)
        self.assertGreater(registers[14 * 4 + 2] & 63, 0)
        for key in ('Space', 'Q'):
            self.emu.request('key', key=key, down=False)
        self.emu.advance(30)
        registers = self.emu.read_memory(PSG, 64, space='vram')
        self.assertTrue(all(registers[voice * 4 + 2] & 63 == 0 for voice in range(12, 16)))
        self.assertEqual(self.game.read('_sound_current_music'), bytes([14]))

    def test_loss_and_human_victory_jingles_end_and_menu_recovers(self):
        for choice, loser, bank, name in ((1, '_gs_player1', 31, 'GAMEOVER.ZSM'),
                                          (2, '_gs_player2', 30, 'VICTORY.ZSM')):
            self.game.start(choice)
            self.game.write(loser, 292, bytes([2]))
            self.emu.advance(4)
            self.assertEqual(self.game.read('_sound_current_music'), bytes([bank]))
            self.assert_music_progresses(name)
            self.emu.advance(300)
            registers = self.emu.read_memory(PSG, 40, space='vram')
            self.assertTrue(all(registers[voice * 4 + 2] & 63 == 0 for voice in range(10)))
            self.game.press('Return')
            self.emu.advance(30)
            self.assertEqual(self.game.read('_sound_current_music'), bytes([10]))


if __name__ == '__main__':
    unittest.main()
