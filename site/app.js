'use strict';
const canvas = document.getElementById('canvas');
const status = document.getElementById('status');
const play = document.getElementById('play');
const audio = document.getElementById('audio');
const player = document.getElementById('player');
const viewport = document.getElementById('game-viewport');
const screen = document.getElementById('screen');
let started = false;
let sound = false;
function fail(message) {
  status.textContent = 'Unable to start the game';
  const error = document.getElementById('error');
  error.textContent = `${message} Reload to try again, or download the game to play in a desktop emulator.`;
  error.hidden = false;
  play.disabled = false;
  play.textContent = 'Reload and try again';
  play.onclick = () => location.reload();
}
// The release's Emscripten runtime reads this global before it starts.
var Module = {
  canvas,
  arguments: ['-prg', 'BBLOCKS.PRG', '-run', '-ram', '512', '-scale', '1', '-quality', 'nearest', '-keymap', 'en-us'],
  locateFile: name => `emulator/${name}`,
  preRun: [function () {
    ENV.SDL_EMSCRIPTEN_KEYBOARD_ELEMENT = '#canvas';
    addRunDependency('battleblocks-manifest');
    fetch('game/manifest.json').then(response => {
      if (!response.ok) throw new Error('The game files could not be downloaded.');
      return response.json();
    }).then(manifest => {
      for (const name of manifest.resources) {
        FS.createPreloadedFile('/', name, `game/${name}`, true, true, undefined,
          () => fail(`The file ${name} could not be downloaded.`));
      }
      removeRunDependency('battleblocks-manifest');
    }).catch(error => fail(error.message));
  }],
  postRun: [function () {
    document.getElementById('cover').hidden = true;
    canvas.hidden = false;
    status.textContent = 'Game running · press Enter for the menu';
    audio.disabled = false;
    document.getElementById('fullscreen').disabled = false;
    document.getElementById('restart').disabled = false;
    Module.ccall('j2c_start_audio', 'void', ['bool'], [false]);
    Module.SDL2?.audioContext?.suspend();
    document.querySelectorAll('[data-key]').forEach(button => { button.disabled = false; });
    updateDisplay();
    canvas.focus({preventScroll: true});
  }],
  print: text => console.log(text),
  printErr: text => console.warn(text),
  setStatus: text => { if (text) status.textContent = 'Loading the emulator…'; },
  onAbort: () => fail('The emulator stopped before the game could start.')
};
play.addEventListener('click', () => {
  if (started) return;
  started = true;
  play.disabled = true;
  play.textContent = 'Loading…';
  status.textContent = 'Loading the game and emulator…';
  const script = document.createElement('script');
  script.src = 'emulator/x16emu.js';
  script.onerror = () => fail('The emulator could not be downloaded.');
  document.body.append(script);
});
canvas.addEventListener('click', () => canvas.focus({preventScroll: true}));
canvas.addEventListener('keydown', event => {
  if (['ArrowUp','ArrowDown','ArrowLeft','ArrowRight',' ','Backspace'].includes(event.key)) event.preventDefault();
});
canvas.addEventListener('contextmenu', event => event.preventDefault());
canvas.addEventListener('webglcontextlost', event => {
  event.preventDefault();
  fail('The browser lost its graphics context.');
});
audio.addEventListener('click', async () => {
  try {
    if (sound) {
      Module.ccall('j2c_start_audio', 'void', ['bool'], [false]);
    } else {
      Module.ccall('j2c_start_audio', 'void', ['bool'], [true]);
      const context = Module.SDL2?.audioContext;
      if (!context) throw new Error('Audio context unavailable');
      await context.resume();
    }
    sound = !sound;
    audio.textContent = sound ? 'Sound on' : 'Sound off';
    audio.setAttribute('aria-pressed', String(sound));
    canvas.focus({preventScroll: true});
  } catch { status.textContent = 'Audio could not be enabled. Try clicking Sound again.'; }
});
document.getElementById('fullscreen').addEventListener('click', async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await player.requestFullscreen();
    canvas.focus({preventScroll: true});
  } catch { status.textContent = 'Fullscreen is unavailable in this browser'; }
});
document.getElementById('restart').addEventListener('click', () => location.reload());
fetch('build.json').then(response => response.json()).then(info => {
  document.getElementById('version').textContent = `v${info.version.replace(/^v/, '')} · ${info.commit.slice(0, 7)}`;
}).catch(() => {});

// Fixed native dimensions in the page; whole physical-pixel multiples in
// fullscreen. Small/touch displays retain a pannable 1x view for legibility.
function updateDisplay() {
  const fullscreen = document.fullscreenElement === player;
  const dpr = window.devicePixelRatio || 1;
  let scale = 1;
  if (fullscreen) {
    const width = viewport.clientWidth, height = viewport.clientHeight;
    const touch = matchMedia('(pointer: coarse)').matches || width < 640;
    scale = touch ? Math.max(1, Math.floor(Math.min(width / 640, height / 480)))
      : Math.max(1, Math.floor(Math.min(width * dpr / 640, height * dpr / 480))) / dpr;
  }
  const width = 640 * scale, height = 480 * scale;
  screen.style.width = canvas.style.width = `${width}px`;
  screen.style.height = canvas.style.height = `${height}px`;
  const border = fullscreen ? 0 : 12;
  screen.style.marginLeft = `${Math.max(0, Math.floor((viewport.clientWidth - width - border) / 2))}px`;
  screen.style.marginTop = `${fullscreen ? Math.max(0, Math.floor((viewport.clientHeight - height) / 2)) : 0}px`;
  screen.style.transform = '';
  // Centered blocks and text above the game can introduce half-pixel origins.
  const rect = screen.getBoundingClientRect();
  const inset = border / 2;
  const x = rect.left + inset, y = rect.top + inset;
  screen.style.transform = `translate(${Math.round(x * dpr) / dpr - x}px, ${Math.round(y * dpr) / dpr - y}px)`;
  if (!started) viewport.scrollLeft = Math.max(0, Math.floor((width + border - viewport.clientWidth) / 2));
}
let displayFrame;
function scheduleDisplay() {
  cancelAnimationFrame(displayFrame);
  displayFrame = requestAnimationFrame(updateDisplay);
}
window.addEventListener('resize', scheduleDisplay);
window.addEventListener('scroll', scheduleDisplay, {passive:true});
document.addEventListener('fullscreenchange', scheduleDisplay);
window.visualViewport?.addEventListener('resize', scheduleDisplay);
new ResizeObserver(scheduleDisplay).observe(player);
updateDisplay();

document.getElementById('exit-fullscreen').addEventListener('click', async () => {
  if (document.fullscreenElement) await document.exitFullscreen();
  canvas.focus({preventScroll:true});
});

// On-screen keys use the same SDL keyboard events as a physical keyboard.
const keyCodes = {Enter:13, Escape:27, ArrowLeft:37, ArrowUp:38, ArrowRight:39, ArrowDown:40, ' ':32, t:84};
const keyNames = {Enter:'Enter', Escape:'Escape', ArrowLeft:'ArrowLeft', ArrowUp:'ArrowUp', ArrowRight:'ArrowRight', ArrowDown:'ArrowDown', ' ':'Space', t:'KeyT'};
function sendKey(key, down) {
  canvas.focus({preventScroll:true});
  const keyCode = keyCodes[key] || key.charCodeAt(0);
  canvas.dispatchEvent(new KeyboardEvent(down ? 'keydown' : 'keyup', {
    key, code:keyNames[key] || `Digit${key}`, keyCode, which:keyCode, bubbles:true
  }));
}
const heldKeys = new Map();
function release(button) {
  const interval = heldKeys.get(button);
  if (interval !== undefined) {
    clearInterval(interval);
    heldKeys.delete(button);
    sendKey(button.dataset.key, false);
  }
}
document.querySelectorAll('[data-key]').forEach(button => {
  button.addEventListener('pointerdown', event => {
    if (button.disabled || heldKeys.has(button)) return;
    event.preventDefault();
    button.setPointerCapture(event.pointerId);
    sendKey(button.dataset.key, true);
    const repeat = ['ArrowLeft','ArrowRight','ArrowDown'].includes(button.dataset.key);
    heldKeys.set(button, repeat ? setInterval(() => sendKey(button.dataset.key, true), 140) : null);
  });
  for (const type of ['pointerup','pointercancel','lostpointercapture']) button.addEventListener(type, () => release(button));
  // Keyboard activation of a focused button produces a click without a pointer.
  button.addEventListener('click', event => {
    if (event.detail === 0 && !button.disabled) {
      sendKey(button.dataset.key, true);
      setTimeout(() => sendKey(button.dataset.key, false), 100);
    }
  });
});
window.addEventListener('blur', () => heldKeys.forEach((_, button) => release(button)));
