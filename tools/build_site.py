"""Build a static demo using the checksum-pinned official r49 WASM release."""
from hashlib import sha256
from pathlib import Path
import json
import shutil
import subprocess
import urllib.request
from zipfile import ZipFile

from package_game import RUNTIME_FILES, version

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE = 'x16emu_wasm-r49.zip'
URL = f'https://github.com/X16Community/x16-emulator/releases/download/r49/{ARCHIVE}'
DIGEST = '9711036cf5b33504815b278fa872b8ec20aef9cb609fd7b17f3685d91312628f'


def main():
    cache = ROOT / 'build/site-cache'
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / ARCHIVE
    if not archive.exists():
        print('Downloading official X16 WebAssembly emulator r49...', flush=True)
        with urllib.request.urlopen(URL, timeout=60) as response:
            data = response.read()
        if sha256(data).hexdigest() != DIGEST:
            raise ValueError('Emulator archive checksum mismatch')
        archive.write_bytes(data)
    if sha256(archive.read_bytes()).hexdigest() != DIGEST:
        raise ValueError(f'Emulator archive checksum mismatch: {archive}')
    output = ROOT / 'build/site'
    shutil.rmtree(output, ignore_errors=True)
    shutil.copytree(ROOT / 'site', output)
    (output / 'emulator').mkdir()
    with ZipFile(archive) as package:
        for name in ('x16emu.js', 'x16emu.wasm', 'x16emu.data'):
            (output / 'emulator' / name).write_bytes(package.read(name))
    (output / 'game').mkdir()
    distribution = ROOT / 'build/CX16-BATTLEBLOCKS.ZIP'
    with ZipFile(distribution) as package:
        if set(package.namelist()) != set(RUNTIME_FILES) | {'VERSION.txt'}:
            raise ValueError('Distribution does not match the runtime resource list')
        for name in RUNTIME_FILES:
            (output / 'game' / name).write_bytes(package.read(name))
    (output / 'game/manifest.json').write_text(json.dumps({'resources': list(RUNTIME_FILES)}, indent=2) + '\n')
    shutil.copy2(distribution, output / distribution.name)
    shutil.copy2(ROOT / 'docs/screenshots/splash.png', output / 'splash.png')
    (output / 'screenshots').mkdir()
    for name in ('light-battle.png', 'cyberpunk-battle.png', 'menu.png', 'options.png', 'foundry-menu.png', 'foundry-battle.png', 'cyberpunk-styles.png', 'arcade-styles.png', 'synthwave-battle.png', 'synthwave-menu.png', 'matrix-battle.png', 'matrix-menu.png'):
        shutil.copy2(ROOT / 'docs/screenshots' / name, output / 'screenshots' / name)
    (output / '.nojekyll').touch()
    result = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=ROOT, capture_output=True, text=True)
    (output / 'build.json').write_text(json.dumps({
        'version': version(), 'commit': result.stdout.strip() if result.returncode == 0 else '',
        'emulator': 'r49'}, indent=2) + '\n')
    print(f'Battle Blocks {version()} website ready: {output}')


if __name__ == '__main__':
    main()
