"""Package only the program and runtime resources, with release metadata."""
from pathlib import Path
import re
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
GRAPHICS_FILES = (
    'TILES.DAT', 'HUD.DAT', 'HUD-A.DAT', 'HUD-F.DAT', 'CYBER.DAT', 'MENU.DAT', 'MENU-A.DAT',
    'ARCADEP.DAT', 'CYBERP.DAT', 'FOUNDRYP.DAT', 'FOUNDRY.DAT', 'FORGE.DAT', 'ARCADE.DAT',
    'MATRIX.DAT', 'MATRIXP.DAT', 'MATBG.DAT', 'HUD-M.DAT', 'NAME-M.DAT', 'NAME-MX.DAT',
    'SYNTH.DAT', 'SYNTHP.DAT', 'HUD-S.DAT', 'NAME-S.DAT', 'NAME-SX.DAT',
    'TITLE.DAT', 'TITLEMAP.DAT', 'TITLEPAL.DAT',
    'NAME-L.DAT', 'NAME-LX.DAT', 'NAME-C.DAT', 'NAME-CX.DAT', 'NAME-F.DAT', 'NAME-FX.DAT',
)

RUNTIME_SOURCES = {
    'AUTOBOOT.X16': 'BBLOCKS.PRG',
    'GRAPHICS.DAT': 'GRAPHICS.DAT',
    'AUDIO.BIN': 'AUDIO.BIN',
}
RUNTIME_FILES = tuple(RUNTIME_SOURCES)


def runtime_source(name):
    """Map a distributed filename to its build output."""
    return ROOT / 'src' / RUNTIME_SOURCES[name]


def version():
    value = (ROOT / 'VERSION').read_text().strip()
    if not re.fullmatch(r'(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)', value):
        raise ValueError('VERSION must contain a major.minor.patch version')
    return value


def main():
    release = version()
    for name in RUNTIME_FILES:
        if not runtime_source(name).is_file():
            raise FileNotFoundError(f'Missing {name}; run make first')
    destination = ROOT / 'build/CX16-BATTLEBLOCKS.ZIP'
    destination.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(destination, 'w', compression=ZIP_DEFLATED) as archive:
        for name in RUNTIME_FILES:
            archive.write(runtime_source(name), name)
        archive.writestr('VERSION.txt', release + '\n')
    with ZipFile(destination) as archive:
        if archive.testzip() is not None:
            raise ValueError('Distribution ZIP failed verification')
    print(f'Battle Blocks {release}: {destination} ({len(RUNTIME_FILES)} runtime files)')


if __name__ == '__main__':
    main()
