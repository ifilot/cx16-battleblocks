"""Verify completeness of the desktop and browser distributions."""
from pathlib import Path
import json
import sys
import unittest
from zipfile import ZipFile
from tempfile import TemporaryDirectory
from unittest.mock import patch

import test_menu_emulator as menu_tests

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from package_game import RUNTIME_FILES, version
from bundle_resources import graphics_bundle, audio_bundle


class DistributionTests(unittest.TestCase):
    @unittest.skipUnless(menu_tests.AVAILABLE, 'Sibling X16 AgentBridge emulator and ROM required')
    def test_extracted_package_runs_all_five_styles(self):
        original_agent = menu_tests.X16Agent
        with TemporaryDirectory(prefix='battleblocks-package-') as directory:
            directory = Path(directory)
            with ZipFile(ROOT / 'build/CX16-BATTLEBLOCKS.ZIP') as archive:
                archive.extractall(directory)

            def packaged_agent(executable, rom, **kwargs):
                return original_agent(executable, rom, fsroot=directory,
                                      options=['-prg', str(directory / 'BBLOCKS.PRG'), '-run'])

            game = menu_tests.MenuTests()
            with patch.object(menu_tests, 'X16Agent', packaged_agent):
                try:
                    game.setUp()
                    for style, name in enumerate(('ARCADE.DAT', 'TILES.DAT', 'FOUNDRY.DAT',
                                                  'SYNTH.DAT', 'MATRIX.DAT')):
                        self.assertEqual(game.read('_screen_style'), bytes([style]))
                        expected = (ROOT / 'src' / name).read_bytes()
                        for offset in range(0, len(expected), 4096):
                            chunk = expected[offset:offset + 4096]
                            self.assertEqual(game.emu.read_memory(offset, len(chunk), space='vram'), chunk)
                        game.press('1')
                        game.press('Return')
                        game.emu.advance(30)
                        game.press('Escape')
                        game.press('T')
                finally:
                    game.doCleanups()

    def test_graphics_bundle_preserves_every_asset_and_index(self):
        data, entries = graphics_bundle()
        self.assertEqual(data, (ROOT / 'src/GRAPHICS.DAT').read_bytes())
        header = (ROOT / 'src/resources_generated.h').read_text()
        for name, offset, size in entries:
            self.assertEqual(data[offset:offset + size], (ROOT / 'src' / name.upper()).read_bytes())
            self.assertIn(f'{{{{{(offset >> 8) & 255}, {offset >> 16}}}, {size}U}}', header)

    def test_audio_bundle_preserves_playback_bank_addresses(self):
        data = (ROOT / 'src/AUDIO.BIN').read_bytes()
        for name, bank in (('SFX.BIN', 2), ('TITLE.ZSM', 10), ('BATTLE.ZSM', 14),
                           ('BATTLE2.ZSM', 18), ('BATTLE3.ZSM', 21), ('BATTLE4.ZSM', 26),
                           ('VICTORY.ZSM', 30), ('GAMEOVER.ZSM', 31)):
            original = (ROOT / 'assets/sound' / name).read_bytes()
            offset = (bank - 2) * 8192
            self.assertEqual(data[offset:offset + len(original)], original)

    def test_audio_cannot_overwrite_dialog_scratch_banks(self):
        original_read = Path.read_bytes
        def oversized_effects(path):
            if path.name == 'SFX.BIN':
                return bytes(8193)
            return original_read(path)
        with patch.object(Path, 'read_bytes', oversized_effects):
            with self.assertRaisesRegex(ValueError, 'reserved dialog banks'):
                audio_bundle()

    @unittest.skipUnless((ROOT / 'build/CX16-BATTLEBLOCKS.ZIP').is_file(), 'Run make dist first')
    def test_package_has_all_runtime_files_and_current_version(self):
        with ZipFile(ROOT / 'build/CX16-BATTLEBLOCKS.ZIP') as archive:
            self.assertIsNone(archive.testzip())
            self.assertEqual(set(archive.namelist()), set(RUNTIME_FILES) | {'VERSION.txt'})
            self.assertEqual(archive.read('VERSION.txt').decode().strip(), version())
            for name in RUNTIME_FILES:
                self.assertEqual(archive.read(name), (ROOT / 'src' / name).read_bytes())

    @unittest.skipUnless((ROOT / 'build/site/build.json').is_file(), 'Run make site first')
    def test_site_has_the_same_runtime_files_and_version(self):
        site = ROOT / 'build/site'
        info = json.loads((site / 'build.json').read_text())
        self.assertEqual(info['version'], version())
        self.assertEqual(info['emulator'], 'r49')
        manifest = json.loads((site / 'game/manifest.json').read_text())
        self.assertEqual(set(manifest['resources']), set(RUNTIME_FILES))
        for name in RUNTIME_FILES:
            self.assertEqual((site / 'game' / name).read_bytes(), (ROOT / 'src' / name).read_bytes())


if __name__ == '__main__':
    unittest.main()
