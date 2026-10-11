/* Boot the actual packaged WASM game under the GitHub Pages project subpath. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const {spawnSync} = require('node:child_process');
const {chromium} = require('playwright');
const root = path.resolve(__dirname, '..');
const site = path.join(root, 'build/site');
const captures = path.join(root, 'build/site-check');
const prefix = '/cx16-battleblocks/';
fs.mkdirSync(captures, {recursive:true});
const mime = {'.html':'text/html','.js':'text/javascript','.css':'text/css','.wasm':'application/wasm','.json':'application/json','.svg':'image/svg+xml','.png':'image/png'};
const server = http.createServer((request,response) => {
  const url = new URL(request.url, 'http://localhost');
  if (!url.pathname.startsWith(prefix)) {response.writeHead(404).end();return;}
  const file = path.resolve(site, url.pathname.slice(prefix.length) || 'index.html');
  if (!file.startsWith(site+path.sep) || !fs.existsSync(file)) {response.writeHead(404).end();return;}
  response.setHeader('Content-Type', mime[path.extname(file)] || 'application/octet-stream');
  response.end(fs.readFileSync(file));
});

(async () => {
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  let browser;
  try {
    browser = await chromium.launch({headless:true, executablePath:process.env.CHROMIUM_PATH || undefined,
      args:['--enable-unsafe-swiftshader']});
    const page = await browser.newPage({viewport:{width:1280,height:1000}});
    // Synthetic standard gamepads exercise SDL and the real KERNAL path.
    await page.addInitScript(() => {
      window.testPads = [0,1].map(index => ({index, id:'Xbox 360 Controller',
        mapping:'standard', connected:true, timestamp:0, axes:[0,0,0,0],
        buttons:Array.from({length:17},()=>({pressed:false,touched:false,value:0}))}));
      Object.defineProperty(navigator,'getGamepads',{value:()=>window.testPads});
    });
    const errors=[], failed=[];
    page.on('pageerror', error=>errors.push(error.message));
    page.on('response', response=>{if(response.status()>=400) failed.push(response.url());});
    page.setDefaultTimeout(30000);
    await page.goto(`http://127.0.0.1:${server.address().port}${prefix}`);
    const styles = ['Commander X16', 'Cyberpunk', 'Foundry', 'Synthwave', 'Matrix'];
    const choices = page.locator('[data-style]');
    assert.equal(await choices.count(), styles.length);
    assert.equal(await page.evaluate(()=>getComputedStyle(document.body).backgroundColor), 'rgb(238, 240, 244)');
    for (let index = 0; index < styles.length; index++) {
      await choices.nth(index).click();
      assert.equal(await page.locator('#style-preview-title').textContent(), styles[index]);
      assert.equal(await choices.nth(index).getAttribute('aria-pressed'), 'true');
      assert.equal(await page.locator('[data-style][aria-pressed="true"]').count(), 1);
      assert.equal(await page.locator('#style-preview').getAttribute('src'), await choices.nth(index).locator('img').getAttribute('src'));
      await page.waitForFunction(()=>document.querySelector('#style-preview').complete && document.querySelector('#style-preview').naturalWidth===640);
    }
    await choices.nth(0).focus();
    await page.keyboard.press('Enter');
    assert.equal(await page.locator('#style-preview-title').textContent(), styles[0]);
    await page.evaluate(()=>scrollTo(0,0));
    await page.screenshot({path:path.join(captures,'website.png'),fullPage:true});
    assert.equal(await page.locator('#version').textContent(),`v${fs.readFileSync(path.join(root,'VERSION'),'utf8').trim()} · ${JSON.parse(fs.readFileSync(path.join(site,'build.json'))).commit.slice(0,7)}`);
    await page.click('#play');
    await page.waitForFunction(()=>typeof FS!=='undefined' && Module.calledRun && !document.querySelector('#canvas').hidden);
    const manifest=JSON.parse(fs.readFileSync(path.join(site,'game/manifest.json')));
    assert.equal(manifest.resources.length,42);
    const loaded=await page.evaluate(()=>FS.readdir('/'));
    for(const file of manifest.resources) assert(loaded.includes(file),`${file} missing from emulator filesystem`);
    assert.deepEqual(await page.evaluate(()=>[canvas.width,canvas.height]),[640,480]);

    async function matches(name, reference, actualBox, expectedBox=actualBox) {
      const capture=path.join(captures,`${name}.png`);
      const deadline=Date.now()+30000;
      while(true) {
        await page.locator('#canvas').scrollIntoViewIfNeeded();
        await page.evaluate(()=>updateDisplay());
        await page.waitForTimeout(50);
        await page.locator('#canvas').screenshot({path:capture});
        const result=spawnSync('python3',['-c',`
import sys
from PIL import Image,ImageChops
actual=Image.open(sys.argv[1]).convert('RGB')
expected=Image.open(sys.argv[2]).convert('RGB')
assert actual.size==(640,480),actual.size
box1=tuple(map(int,sys.argv[3:7]));box2=tuple(map(int,sys.argv[7:11]))
# Chromium element screenshots can blend the outermost canvas rows with CSS
# borders. Compare the native artwork inside those rows, including all glyphs.
if box1[1]==0:
    box1=(box1[0],1,box1[2],box1[3])
    box2=(box2[0],box2[1]+1,box2[2],box2[3])
if box1[3]==480:
    box1=(box1[0],box1[1],box1[2],479)
    box2=(box2[0],box2[1],box2[2],box2[3]-1)
assert ImageChops.difference(actual.crop(box1),expected.crop(box2)).getbbox() is None
`,capture,path.join(root,reference),...actualBox.map(String),...expectedBox.map(String)],{encoding:'utf8'});
        if(result.status===0) return;
        assert(!result.error,result.error?.message);
        if(Date.now()>=deadline) assert.fail(`${name} did not match approved artwork: ${capture}\n${result.stderr}`);
        await page.waitForTimeout(300);
      }
    }
    const banner=(name,style,role,x)=>matches(name,`tests/fixtures/banners/${style}-${role}.png`,[x,0,x+274,28],[0,0,274,28]);
    async function pad(index,button) {
      await page.evaluate(([index,button]) => {
        testPads[index].buttons[button]={pressed:true,touched:true,value:1};
        testPads[index].timestamp=performance.now();
      },[index,button]);
      await page.waitForTimeout(150);
      await page.evaluate(([index,button]) => {
        testPads[index].buttons[button]={pressed:false,touched:false,value:0};
        testPads[index].timestamp=performance.now();
      },[index,button]);
      await page.waitForTimeout(150);
    }
    await matches('splash','docs/screenshots/splash.png',[0,0,640,480]);
    await pad(1,9); // Controller 2 Start dismisses the splash.
    await matches('menu','docs/screenshots/menu.png',[0,0,640,368]);
    await page.locator('#canvas').press('5',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await matches('style-menu','docs/screenshots/arcade-styles.png',[0,0,640,480]);
    await page.locator('#canvas').press('2',{delay:120});
    await page.locator('#canvas').press('Escape',{delay:120});
    await matches('style-cancel','docs/screenshots/menu.png',[0,0,640,128]);
    await page.locator('#canvas').press('Enter',{delay:120});
    await page.locator('#canvas').press('3',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await page.locator('#canvas').press('2',{delay:120});
    await matches('foundry-menu','docs/screenshots/foundry-menu.png',[0,0,640,368]);
    await page.locator('#canvas').press('Enter',{delay:120});
    await page.waitForTimeout(800);
    await page.locator('#canvas').press('Escape',{delay:120});
    await matches('foundry-return','docs/screenshots/foundry-menu.png',[0,0,640,368]);
    await page.locator('#canvas').press('t',{delay:120});
    await matches('synthwave-menu','docs/screenshots/synthwave-menu.png',[0,0,640,368]);
    await page.locator('#canvas').press('5',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await matches('synthwave-styles','docs/screenshots/synthwave-styles.png',[0,0,640,480]);
    await page.locator('#canvas').press('4',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await page.locator('#canvas').press('2',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await banner('synthwave-player','synthwave','player',16);
    await banner('synthwave-computer','synthwave','computer',336);
    await page.locator('#canvas').press('Escape',{delay:120});
    await page.locator('#canvas').press('t',{delay:120});
    await matches('matrix-menu','docs/screenshots/matrix-menu.png',[0,0,640,368]);
    await page.locator('#canvas').press('5',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await page.locator('#canvas').press('5',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await page.locator('#canvas').press('2',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await banner('matrix-player','matrix','player',16);
    await banner('matrix-computer','matrix','computer',336);
    await page.locator('#canvas').press('Escape',{delay:120});
    await page.locator('#canvas').press('Escape',{delay:120});
    await matches('matrix-stays-in-menu','docs/screenshots/matrix-menu.png',[0,0,640,368]);
    await page.locator('#canvas').press('t',{delay:120});
    await page.waitForTimeout(500);
    await page.locator('#canvas').press('2',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await banner('light-battle','light','player',16);
    await banner('light-opponent','light','computer',336);
    await page.click('#audio');
    assert.equal(await page.locator('#audio').getAttribute('aria-pressed'),'true');
    assert.equal(await page.evaluate(()=>Module.SDL2.audioContext.state),'running');
    await page.click('#audio');
    assert.equal(await page.locator('#audio').getAttribute('aria-pressed'),'false');
    await page.locator('#canvas').press('Escape',{delay:120});
    await page.waitForTimeout(600);
    await page.locator('#canvas').press('3',{delay:120});
    await pad(0,9); // Controller 1 Start launches the local battle.
    await matches('controller-local','tests/fixtures/banners/light-player.png',
      [336,0,372,28],[0,0,36,28]);
    // Standard gamepad B maps to SNES A: hard-drop on player 2 only.
    await pad(1,1);
    await page.locator('#canvas').screenshot({path:path.join(captures,'controller-drop.png')});
    const controllerDrop=spawnSync('python3',['-c',`
import sys
from PIL import Image
image=Image.open(sys.argv[1]).convert('RGB')
colors={(51,204,238),(238,221,68),(153,102,204),(85,187,119),
        (221,68,85),(85,136,204),(221,85,170)}
for x,occupied in ((16,False),(336,True)):
    pixels=set(image.crop((x,380,x+160,447)).getdata())
    assert bool(pixels & colors)==occupied,(x,pixels & colors)
`,path.join(captures,'controller-drop.png')],{encoding:'utf8'});
    assert.equal(controllerDrop.status,0,controllerDrop.stderr);
    await pad(1,8); // Controller 2 Select returns to the menu.
    await matches('controller-return','docs/screenshots/menu.png',[0,0,640,128]);
    await page.locator('#canvas').press('t',{delay:120});
    await page.waitForTimeout(500);
    await page.locator('#canvas').press('4',{delay:120});
    await page.locator('#canvas').press('Enter',{delay:120});
    await banner('cyberpunk-demo','cyberpunk','computer',16);
    // Actual AI gameplay must leave colored locked pieces on the lower board.
    const deadline=Date.now()+20000;
    while(true) {
      const capture=path.join(captures,'demo-progress.png');
      await page.locator('#canvas').screenshot({path:capture});
      const result=spawnSync('python3',['-c',`
import sys
from PIL import Image
pixels=Image.open(sys.argv[1]).convert('RGB').crop((16,300,176,450))
colors={(255,51,204),(221,136,255),(68,136,255),(0,238,255),(34,255,136),(255,238,34),(255,34,68)}
assert sum(p in colors for p in pixels.getdata())>100
`,capture],{encoding:'utf8'});
      if(result.status===0)break;
      if(Date.now()>=deadline)assert.fail('Demo AI did not place any pieces');
      await page.waitForTimeout(500);
    }
    await page.setViewportSize({width:1920,height:1080});
    await page.click('#fullscreen');
    await page.waitForFunction(()=>document.fullscreenElement?.id==='player');
    await page.waitForTimeout(300);
    let box=await page.locator('#canvas').boundingBox();
    assert.equal(box.width,1280);assert.equal(box.height,960);
    const fullscreen=path.join(captures,'fullscreen.png');
    await page.locator('#canvas').screenshot({path:fullscreen});
    const pixels=spawnSync('python3',['-c',`
import sys,numpy as np
from PIL import Image
p=np.asarray(Image.open(sys.argv[1]).convert('RGB'))
assert p.shape==(960,1280,3)
blocks=p.reshape(480,2,640,2,3)
assert np.all(blocks==blocks[:,0:1,:,0:1,:]),'Fullscreen interpolates pixels'
`,fullscreen],{encoding:'utf8'});
    assert.equal(pixels.status,0,pixels.stderr);
    await page.evaluate(()=>document.exitFullscreen());
    await page.setViewportSize({width:390,height:900});
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'Mobile page overflows');
    await page.locator('[data-style="matrix"]').click();
    assert.equal(await page.locator('#style-preview-title').textContent(), 'Matrix');
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'Mobile style preview overflows');
    assert.equal((await page.locator('#canvas').boundingBox()).width,640);
    assert(await page.locator('#touch-keys').isVisible());
    // Touch keyboard must navigate out of the match and back into a solo game.
    async function touchKey(key) {
      const target=page.locator(`[data-key=${JSON.stringify(key)}]`);
      await target.scrollIntoViewIfNeeded();
      const box=await target.boundingBox();
      await page.mouse.move(box.x+box.width/2,box.y+box.height/2);
      await page.mouse.down();
      await page.waitForTimeout(120);
      await page.mouse.up();
      await page.waitForTimeout(400);
    }
    await touchKey('Escape');
    await touchKey('1');
    await touchKey('Enter');
    await banner('touch-solo','cyberpunk','player',16);
    await page.screenshot({path:path.join(captures,'mobile.png'),fullPage:true});
    await page.click('#restart');
    await page.waitForFunction(()=>!document.querySelector('#play').disabled && !document.querySelector('#cover').hidden);
    assert.deepEqual(errors,[]);assert.deepEqual(failed,[]);
    console.log('PASS: WASM boot, 42 runtime files, two gamepads, five accessible style previews, exact splash/menu/banners, real AI demo, theme switching, audio, integer fullscreen pixels, mobile controls and restart under the Pages subpath.');
  } finally {if(browser)await browser.close();server.close();}
})().catch(error=>{console.error(error);process.exitCode=1;server.close();});
