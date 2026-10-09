Battle Blocks audio

These generated assets come from cx16-sound-generator/assets/battleblocks.
They are bundled here so building the game does not require the generator.

Title and menu: Neon Skyline (TITLE.ZSM). Moving between these screens keeps
the same tune playing; the intro is not restarted.

Each match, including single player and computer demo, uses the next battle
track in this order: Grid Battle, Circuit Rush, Overclock, Skyline Anthem.
The sequence repeats. Each track plays its intro once and loops afterwards.

A human winner gets VICTORY.ZSM. Single-player loss, a computer victory,
and the computer demo get GAMEOVER.ZSM. Both jingles end by themselves.

Effects cover title/menu navigation, valid movement and rotation, landing,
hard drop, single/double/triple/four-line clears, garbage sent/received,
and level increases. AI movement and rotation are silent. Effects for each
player have their own playback priority and voices, allowing simultaneous
events. Within one player's slot, a newer event replaces the previous effect.

All files are loaded once at startup into banked RAM: ZSMKit state in bank 1,
effects in bank 2, music from bank 10 (see music.h for the generated layout).
The ISR plays directly from memory; no disk streaming or per-frame filling
is needed. Music attenuation is 4, leaving room for effects in the mix.
Missing audio files disable playback so the game can still run.

After rebuilding sounds in the generator, run `make import-sounds` here.
To import another generated folder, use:
  make import-sounds SOUND_SOURCE=/path/to/generated/psg
Then run `make run` as usual. Imports check ZSM headers and bank boundaries.

ZSMKit v1 is the same library used by cx16-kakuro. It is by mooinglemur,
under the MIT license; the copyright and license are in src/zsmkit.inc.
Source: https://github.com/mooinglemur/zsmkit
