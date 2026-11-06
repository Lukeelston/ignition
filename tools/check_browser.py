"""Exercise real Chrome audio, patch files, WAV download and responsive layout."""
import json
from pathlib import Path
import subprocess
import sys
import wave
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'evidence'
OUT.mkdir(exist_ok=True)


def main():
    server = subprocess.Popen([sys.executable, str(ROOT / 'serve.py'), '--no-browser', '--port', '0'], cwd=ROOT, stdout=subprocess.PIPE, text=True)
    browser = None
    report = {'server_pid': server.pid}
    try:
        address = json.loads(server.stdout.readline())['url']
        report['url'] = address
        with sync_playwright() as pw:
            browser = pw.chromium.launch(channel='chrome', headless=True)
            page = browser.new_page(viewport={'width': 1440, 'height': 1080}, device_scale_factor=1)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.add_init_script('''
                const Original = window.AudioContext;
                window.AudioContext = class extends Original {
                    constructor(...args) { super(...args); window.testContext = this; }
                };
                const connect = AudioNode.prototype.connect;
                AudioNode.prototype.connect = function (...args) {
                    if (this instanceof AudioWorkletNode) window.testSource = this;
                    return connect.apply(this, args);
                };
            ''')
            page.goto(address)
            page.get_by_role('heading', name='Ignition', exact=True).wait_for(timeout=5000)
            assert page.locator('#presets button').count() == 8
            page.screenshot(path=str(OUT / 'ignition-desktop.png'), full_page=True)
            page.get_by_role('button', name='Start engine', exact=True).click()
            page.wait_for_function("document.body.dataset.running === 'true'")
            capture = page.evaluate('''async () => {
                const context = window.testContext;
                const analyser = context.createAnalyser(); analyser.fftSize = 2048;
                window.testSource.connect(analyser);
                await new Promise(r => setTimeout(r, 300));
                const data = new Float32Array(2048); analyser.getFloatTimeDomainData(data);
                analyser.disconnect();
                return {state:context.state, peak: Math.max(...data.map(Math.abs)), rms: Math.sqrt(data.reduce((s,x)=>s+x*x,0)/data.length)};
            }''')
            assert capture['state'] == 'running' and capture['rms'] > .001, capture
            report['live_audio'] = capture
            live_wav = page.evaluate('''async () => {
                const context=window.testContext, engineSource=window.testSource;
                const source=`class Capture extends AudioWorkletProcessor {
                    constructor(){super();this.buffer=new Float32Array(48000);this.offset=0;}
                    process(inputs){const data=inputs[0]?.[0];if(data){
                        const count=Math.min(data.length,this.buffer.length-this.offset);
                        this.buffer.set(data.subarray(0,count),this.offset);this.offset+=count;
                        if(this.offset===this.buffer.length){this.port.postMessage(this.buffer,[this.buffer.buffer]);return false;}
                    }return true;}
                }registerProcessor('qa-capture',Capture);`;
                const url=URL.createObjectURL(new Blob([source],{type:'text/javascript'}));
                await context.audioWorklet.addModule(url);URL.revokeObjectURL(url);
                const recorder=new AudioWorkletNode(context,'qa-capture');
                const pending=new Promise(resolve=>recorder.port.onmessage=event=>resolve(event.data));
                engineSource.connect(recorder);recorder.connect(context.destination);
                const samples=await pending;engineSource.disconnect(recorder);recorder.disconnect();
                const {encodeWav}=await import('./engine.js');
                return Array.from(new Uint8Array(encodeWav(samples,context.sampleRate)));
            }''')
            (OUT / 'live-worklet-capture.wav').write_bytes(bytes(live_wav))
            report['live_capture_file'] = 'live-worklet-capture.wav'
            page.locator('#rpm').fill('4200')
            page.locator('#rpm').dispatch_event('input')
            page.wait_for_function("Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) > 3500")
            page.screenshot(path=str(OUT / 'ignition-running.png'), full_page=True)
            page.get_by_role('button', name='Mute', exact=True).click()
            page.wait_for_function("document.querySelector('#output-level').textContent === '— dBFS'")
            page.get_by_role('button', name='Unmute', exact=True).click()
            page.get_by_role('button', name='Hold to rev', exact=True).focus()
            page.keyboard.down('Space')
            page.wait_for_timeout(200)
            page.keyboard.up('Space')
            assert page.locator('#rev').get_attribute('aria-pressed') == 'false'
            page.locator('#rev').hover()
            page.mouse.down()
            page.locator('#rev').dispatch_event('pointercancel', {'pointerId': 1})
            page.mouse.up()
            assert page.locator('#rev').get_attribute('aria-pressed') == 'false'
            with page.expect_download() as saved:
                page.get_by_role('button', name='Save patch', exact=True).click()
            saved.value.save_as(OUT / 'browser-patch.json')
            patch = json.loads((OUT / 'browser-patch.json').read_text())
            assert patch['patch']['rpm'] == 4200
            page.get_by_role('button', name='Ion drive', exact=True).click()
            page.locator('#patch-file').set_input_files(str(OUT / 'browser-patch.json'))
            page.wait_for_function("document.querySelector('#rpm').value === '4200'")
            page.locator('#duration').fill('2')
            page.locator('#motion').select_option('rev')
            with page.expect_download(timeout=20000) as rendered:
                page.get_by_role('button', name='Render WAV', exact=True).click()
            rendered.value.save_as(OUT / 'browser-render.wav')
            with wave.open(str(OUT / 'browser-render.wav')) as wav:
                assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()) == (1, 3, 48000, 96000)
                report['export'] = {'channels': 1, 'bit_depth': 24, 'sample_rate': 48000, 'frames': 96000}
            page.locator('#preview').evaluate('(audio) => audio.play()')
            page.wait_for_function("document.body.dataset.running === 'false'")
            page.get_by_role('button', name='Start engine', exact=True).click()
            page.wait_for_function("document.body.dataset.running === 'true'")
            assert page.locator('#preview').evaluate('(audio) => audio.paused'), 'Starting the engine must stop preview playback'
            page.get_by_role('button', name='Stop engine', exact=True).click()
            page.wait_for_function("document.body.dataset.running === 'false'")
            assert page.evaluate('window.testContext.state') == 'closed'
            page.get_by_role('button', name='Start engine', exact=True).click()
            page.wait_for_function("document.body.dataset.running === 'true'")
            page.get_by_role('button', name='Stop engine', exact=True).click()
            page.wait_for_function("document.body.dataset.running === 'false'")
            page.reload()
            page.wait_for_function("document.querySelector('#rpm')?.value === '4200'", timeout=5000)
            page.set_viewport_size({'width': 390, 'height': 844})
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth'), 'Mobile overflow'
            page.screenshot(path=str(OUT / 'ignition-mobile.png'), full_page=True)
            assert not errors, errors
            report['browser_errors'] = errors
            page.get_by_role('button', name='Quit app', exact=True).click()
            server.wait(timeout=5)
            browser.close(); browser = None
        report['passed'] = True
    finally:
        if browser:
            try: browser.close()
            except Exception: pass
        if server.poll() is None:
            server.terminate()
            server.wait(timeout=5)
        report['server_stopped'] = server.poll() is not None
        (OUT / 'browser-report.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
