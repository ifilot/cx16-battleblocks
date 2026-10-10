"""Keep the game's About screen version in sync with release metadata."""
from pathlib import Path

from package_game import version

output = Path(__file__).resolve().parents[1] / 'src/version_generated.h'
content = f'/* Generated from VERSION. */\n#define GAME_VERSION "{version()}"\n'
if not output.is_file() or output.read_text() != content:
    output.write_text(content)
