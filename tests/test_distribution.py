"""Verify completeness of the desktop and browser distributions."""
from pathlib import Path
import json
import sys
import unittest
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from package_game import RUNTIME_FILES, version


class DistributionTests(unittest.TestCase):
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
