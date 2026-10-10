# Battle Blocks

Current version: **0.1.0** ([VERSION](VERSION)).

A falling-block battle game for the **Commander X16**. Play solo, challenge the computer, compete locally with a friend, or watch two computers battle in demo mode.

Clearing two, three, or four lines sends **one, two, or four garbage rows** to the opponent. Levels increase every ten cleared lines. Each board tracks its score, pieces, and single/double/triple/four-line clears.

Choose between **Light** and **Cyberpunk** themes, with bundled music and sound effects.
Scene changes show a built-in `LOADING...` label: white on black in Classic,
cyan on dark navy in Cyberpunk. Each scene appears as soon as its assets are
ready, with no animation delay or extra loading-screen files.

## Screenshots

Captured directly from the emulator at **640×480**. The splash uses exactly **16 colors**; both gameplay screenshots show real computer-versus-computer demo matches.

| Start screen | Menu |
| --- | --- |
| ![Isometric Block Metropolis splash](docs/screenshots/splash.png) | ![Game modes and theme selection](docs/screenshots/menu.png) |

| Light theme | Cyberpunk theme |
| --- | --- |
| ![Two computers battling with Light cards](docs/screenshots/light-battle.png) | ![Cyberpunk battle with the Microchip circuit-board background](docs/screenshots/cyberpunk-battle.png) |

| About | Audio options |
| --- | --- |
| ![Version, author, hardware details, and source repository](docs/screenshots/about.png) | ![Independent music and sound effect controls](docs/screenshots/options.png) |

| Classic loading | Cyberpunk loading |
| --- | --- |
| ![White loading text on black](docs/screenshots/loading.png) | ![Cyan loading text on navy](docs/screenshots/loading-cyberpunk.png) |

## Build and run

Requires **cc65** (`cl65`), **Python 3**, **Pillow**, and **NumPy**, plus the X16 emulator and a compatible ROM.

```sh
python3 -m pip install Pillow numpy
make run
```

The emulator is found in `../x16-emulator/build/x16emu`, falling back to
`~/emulator-linux/x16emu`. The ROM is found beside the selected emulator, then
in `../emulator/rom.bin`, then in `~/emulator-linux/rom.bin`. Override their
locations when needed (relative paths are resolved from where you run make):

```sh
make run EMU=/path/to/x16emu ROM=/path/to/rom.bin
```

Use `make` to build without launching, or `make clean` to remove generated game files.
Build and launch output shows compact status lines; compiler and emulator
diagnostics remain visible. Use `make run V=1` to also show the full commands.
`make -C src run` uses the same launcher and defaults.

Use `make dist` to create `build/CX16-BATTLEBLOCKS.ZIP`, containing 19 runtime
files (the program, graphics, and audio) plus `VERSION.txt`. Keep them together.

## Record gameplay demos

Run `make demo` to record two looping GIFs at **640×480 and 10 fps**:

- [Regular (Light) style](img/cx16-battleblocks-gameplay.gif)
- [Cyberpunk style](img/cx16-battleblocks-cyberpunk-gameplay.gif)

Each shows the title, menu selection, 30 seconds of real computer-versus-computer
gameplay, and a return to the menu and title. Recording runs without a window
using temporary copies of the runtime files. It requires the AgentBridge
emulator and Python client from the neighboring `x16-emulator` repository,
plus Pillow. The usual `EMU`, `ROM`, and `V=1` overrides apply.

Set `DEMO_GIF` and `DEMO_CYBERPUNK_GIF` to change the output paths. If the
AgentBridge client is elsewhere, run the recorder directly with `--api-dir`:

```sh
python3 tools/record_gameplay.py --emulator /path/to/x16emu --rom /path/to/rom.bin \
  --api-dir /path/to/api/python
```

## Browser demo

The website runs the actual game using the official Commander X16 WebAssembly
emulator r49. It includes sound, fullscreen with integer pixel scaling, touch
controls, and a downloadable desktop package.

```sh
make site
python3 -m http.server 8000 --directory build/site
```

Open <http://localhost:8000>. The first site build downloads a checksum-verified
emulator archive; subsequent builds reuse `build/site-cache/`.

## GitHub Actions and releases

- `build.yml` compiles the game, checks the assets and package, and uploads the
  distribution on pushes to `master`/`develop`, pull requests, and manual runs.
- Tags named `vX.Y.Z` must match `VERSION`. A tag such as `v0.1.0` publishes a
  GitHub release with `CX16-BATTLEBLOCKS.ZIP` attached.
- `pages.yml` builds and checks the browser demo in Chromium, including real
  gameplay, keyboard input, theme changes, audio, and fullscreen rendering.
  Pull requests are checked; pushes to `master` deploy to GitHub Pages.

In repository **Settings → Pages → Build and deployment**, select
**GitHub Actions** as the source. The deployed demo will be available at
<https://ifilot.github.io/cx16-battleblocks/>.

To run the browser checks locally:

```sh
npm install --prefix build/browser-tools playwright@1.64.0
build/browser-tools/node_modules/.bin/playwright install chromium
NODE_PATH="$PWD/build/browser-tools/node_modules" node tests/check_site.cjs
```

`VERSION` supplies the release package and website metadata. After changing it,
run `make site` to regenerate both. Native AgentBridge tests also run when the
sibling desktop emulator and ROM are available.

## Controls

| Action | Player 1 | Player 2 (local battle) |
| --- | --- | --- |
| Move | Left / Right | A / D |
| Rotate | Up | W |
| Soft drop | Down | S |
| Hard drop | Space or 0 | Q |

In the menu, use Up/Down to select and Enter/Space to confirm. Number keys **1–7**
select an option; **T** switches themes. **6** selects Options and **7** selects
About. **Escape** returns from a game to the menu, and from the menu to the splash.

In Options, select Music or Sounds with Up/Down. Enter/Space turns that channel
on or off; Left/Right adjusts its volume from **0 to 100%** in ten steps. Changes
apply immediately, and turning a channel off preserves its volume. Escape or
the Back entry returns to the main menu. Settings last for the current session,
including games and visits to the splash. The defaults are 90% music and 100%
effects, matching the original playback levels.

About shows the version from `VERSION` (currently **v0.1.0**), author **Ivo Filot**,
the GitHub repository, and CPU, graphics, audio, and compiler details.

## Refresh screenshots

With the sibling emulator's AgentBridge Python API available:

```sh
make
python3 tools/capture_screenshots.py
```

Screenshots live in `docs/screenshots/`; production game assets remain in `assets/`.
