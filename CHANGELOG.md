# Changelog

Release history reconstructed from Git tags, commits, and the changes in the
working tree. Versions are listed newest first.

## 0.2.4 — 2026-10-13

### Changed

- Computer difficulty moved from Options to the main menu, shown above the
  pieces beside the computer's face. Left/Right (or A/D, or the D-pad) change
  it directly; Options now holds only music and sound settings.
- The menu piece panels leave a free column inside both edges: the I piece no
  longer touches the left frame, and the right column no longer touches the
  right frame. The style preview panel is one column narrower to match.
- The selection cursor is smaller, matching the menu font's capital height
  instead of filling its whole tile.
- The screenshot tool confirms the quit dialog when leaving each demo match and
  accepts `EMU`/`ROM` overrides, matching the emulator tests.

## 0.2.3 — 2026-10-11

### Added

- **Easy**, **Normal**, and **Hard** computer difficulty in Options, retained
  for the session. Easy is the default; slower movement and a wait before
  hard drops give the opponent more time to react. Hard keeps the old speed.

### Changed

- Release ZIPs name the executable **AUTOBOOT.X16**, so users can launch it
  with **BOOT**, or automatically from the SD card root. The package still
  contains only four files; browser and recording tools use the same filename.
- The first main-menu item, **Solo**, is selected on startup. Returning from
  a match still selects the mode that was just played.
- Expanded the options menu to include computer difficulty alongside audio.
- Compacted nonrecursive menu and match code using static local storage to
  fit the new options while preserving the software stack reservation.

## 0.2.2 — 2026-10-11

### Changed

- SNES **Start** now pauses and resumes a match from either controller, with a
  visible pause indicator. Both boards, AI, gravity, and row-clear animations
  stop while paused, without a timing jump on resume.
- SNES **Select** is ignored during gameplay and pause; it still goes back or
  cancels a selection in menus.
- Keyboard **Escape** opens an in-game quit-confirmation dialog with **No**
  selected. Canceling restores the match and its previous pause state;
  keyboard and controller input can confirm or cancel the dialog.
- Compacted the graphics resource index and loader to make room for controller
  improvements within the game's RAM budget.

### Fixed

- The quit dialog's **Yes** label shifted when switching between **Yes** and
  **No**. Both labels now stay fixed while the selection cursor moves.
- Duplicate controller actions caused by brief button-release glitches. Action
  buttons now use approximately 33–50 ms of release debouncing while responding
  immediately to the first press; directional repeat keeps its existing timing.
- Simultaneous **Start** presses on both controllers could immediately resume a
  newly paused match.
- Sliding a piece beneath an overhang could project its landing shadow above
  the overhang and leave shadow tiles behind after the next piece spawned.
  Shadows now follow downward collisions from the piece's current position.
- Hard drop could place a tucked piece above an overhang instead of inside the
  gap below it.

### Added

- Emulator regression checks for pause/resume, quit confirmation and screen
  restoration, ignored Select input, button bounce, and shadows and hard drops
  beneath overhangs, including collision
  comparisons for all seven pieces in all four rotations.
- Updated the AI benchmark harness to link the bundled resource loader and
  accept a ROM path supplied through `ROM`.

## 0.2.1 — 2026-10-11

### Changed

- Reduced the distribution ZIP from 43 files to four: `BBLOCKS.PRG`,
  `GRAPHICS.DAT`, `AUDIO.BIN`, and `VERSION.txt`.
- Bundled graphics into one indexed resource file and audio into one
  bank-aligned image, retaining all five styles, music, and sound effects.
- Updated the game loader, browser distribution, recording tools, and build
  documentation to use the bundled resources.

### Added

- Bundle-integrity checks and an emulator test that runs the extracted release
  package through all five styles.

## 0.2.0 — 2026-10-11

### Added

- Foundry, Synthwave, and Matrix styles, bringing the total to five.
- A dedicated style-selection menu with previews and a selected-style marker.
- Two-port SNES controller support for gameplay and menu navigation, including
  held-direction repeat and press-only action buttons.
- Independent music and sound-effect volume controls and an About screen with
  release metadata.
- Gameplay recordings and broader native and browser regression coverage.

### Changed

- Made Commander X16 the default style and refreshed board grids, Foundry
  frames, menu lettering, and themed player/computer nameplates.
- Refreshed the browser showcase, screenshots, and gameplay documentation to
  present all five styles.
- Improved menu navigation and scene transitions, including style previews
  that restore the original style when canceled.

## 0.1.0 — 2026-10-09

### Added

- First tagged release of Battle Blocks for the Commander X16.
- Single-player, versus-computer, local two-player, and computer-demo modes.
- Falling pieces, rotation, soft and hard drop, landing shadows, next-piece
  previews, animated line clears, scoring, levels, and match statistics.
- Garbage attacks between opposing boards and computer placement searches.
- Light and Cyberpunk themes, a splash screen, music, and sound effects.
- A browser demo using the official r49 WebAssembly emulator, with sound,
  fullscreen, touch controls, and a downloadable desktop package.
- Build, distribution, GitHub release, and GitHub Pages workflows.
