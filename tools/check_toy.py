"""Exercise the playable toy through real Chrome controls and capture live audio."""
from pathlib import Path
import json
import subprocess
import sys
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'evidence'


def main():
    server = subprocess.Popen([sys.executable, str(ROOT / 'serve.py'), '--no-browser', '--port', '0'], stdout=subprocess.PIPE, text=True)
    report = {}
    try:
        url = json.loads(server.stdout.readline())['url']
        with sync_playwright() as pw:
            browser = pw.chromium.launch(channel='chrome', headless=True)
            try:
                page = browser.new_page(viewport={'width': 1280, 'height': 1100})
                errors = []
                page.on('pageerror', lambda error: errors.append(str(error)))
                page.add_init_script('''
                  const Original=window.AudioContext;
                  window.AudioContext=class extends Original {constructor(...args){super(...args);window.qaContext=this;}};
                  const connect=AudioNode.prototype.connect;
                  AudioNode.prototype.connect=function(...args){if(this instanceof AudioWorkletNode)window.qaSource=this;return connect.apply(this,args);};
                ''')
                page.goto(url)
                assert page.locator('body').get_attribute('data-view') == 'play'
                assert not page.locator('#designer-title').is_visible()
                page.get_by_role('button', name='Start engine', exact=True).click()
                page.wait_for_function("document.querySelector('#state-label').textContent === 'Running'")
                page.wait_for_function("Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) > 700")
                assert page.locator('#gear-readout').inner_text() == 'N'
                page.locator('#trigger').focus()
                page.keyboard.down('Space')
                page.wait_for_function("Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) > 5500")
                assert page.locator('#speed-readout').inner_text() == '0'
                page.keyboard.up('Space')
                assert page.locator('#throttle-value').inner_text() == '0%'
                page.get_by_role('button', name='Gear up', exact=True).click()
                page.locator('#trigger').focus()
                page.keyboard.down('Space')
                # The tachometer follows audible/smoothed RPM. Let first gear
                # actually rev before shifting; road speed alone outruns it.
                page.wait_for_function("Number(document.querySelector('#speed-readout').textContent) >= 40 && Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) > 6000")
                before = int(page.locator('#rpm-readout').text_content().replace(',', ''))
                page.keyboard.press('ArrowUp')
                page.wait_for_function("document.querySelector('#gear-readout').textContent === '2'")
                # Keep the trigger held through the shift; changing gear must not release it.
                assert page.locator('#throttle-value').inner_text() == '100%'
                # Observe the shift dip itself; the uprated motor can recover
                # much of it within the old fixed 650 ms sampling delay.
                page.wait_for_function("limit => Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) < limit", arg=before*.9, timeout=1200)
                after = int(page.locator('#rpm-readout').text_content().replace(',', ''))
                assert after < before * .9, (before, after)
                report['upshift_rpm'] = [before, after]
                live = page.evaluate('''async () => {
                  const context=window.qaContext, source=window.qaSource;
                  const code=`class ToyCapture extends AudioWorkletProcessor {
                    constructor(){super();this.data=new Float32Array(sampleRate*2);this.at=0;}
                    process(inputs){const x=inputs[0]?.[0];if(x){const n=Math.min(x.length,this.data.length-this.at);this.data.set(x.subarray(0,n),this.at);this.at+=n;if(this.at===this.data.length){this.port.postMessage(this.data,[this.data.buffer]);return false;}}return true;}
                  }registerProcessor('toy-capture',ToyCapture);`;
                  const url=URL.createObjectURL(new Blob([code],{type:'text/javascript'}));
                  await context.audioWorklet.addModule(url);URL.revokeObjectURL(url);
                  const recorder=new AudioWorkletNode(context,'toy-capture');
                  const pending=new Promise(resolve=>recorder.port.onmessage=e=>resolve(e.data));
                  const silent=context.createGain();silent.gain.value=0;
                  source.connect(recorder);recorder.connect(silent);silent.connect(context.destination);
                  const samples=await pending;source.disconnect(recorder);recorder.disconnect();silent.disconnect();
                  let energy=0,peak=0;for(const x of samples){energy+=x*x;peak=Math.max(peak,Math.abs(x));}
                  const {encodeWav}=await import('./engine.js');
                  return {rms:Math.sqrt(energy/samples.length),peak,wav:Array.from(new Uint8Array(encodeWav(samples,context.sampleRate)))};
                }''')
                (OUT / 'toy-live-capture.wav').write_bytes(bytes(live.pop('wav')))
                assert live['rms'] > .001 and live['peak'] < 1, live
                report['live_capture'] = live
                page.keyboard.up('Space')
                speed = int(page.locator('#speed-readout').inner_text())
                page.keyboard.down('b')
                page.wait_for_timeout(1000)
                assert int(page.locator('#speed-readout').inner_text()) < speed - 15
                page.keyboard.up('b')
                assert page.locator('#brake').get_attribute('aria-pressed') == 'false'
                page.locator('#boost').check()
                assert page.locator('#drive-indicator').inner_text() == 'BOOST ON'
                page.locator('#trigger').hover()
                page.mouse.down()
                page.locator('#trigger').dispatch_event('pointercancel', {'pointerId': 1})
                page.mouse.up()
                assert page.locator('#throttle-value').inner_text() == '0%'
                page.locator('#throttle').fill('0.45')
                page.locator('#throttle').dispatch_event('input')
                assert page.locator('#throttle-value').inner_text() == '45%'
                page.locator('#throttle').dispatch_event('pointerup', {'pointerId': 1})
                assert page.locator('#throttle-value').inner_text() == '0%'
                page.locator('#trigger').focus()
                page.keyboard.down('Space')
                page.evaluate("window.dispatchEvent(new Event('blur'))")
                assert page.locator('#throttle-value').inner_text() == '0%'
                page.keyboard.up('Space')
                # The keyup can occur outside the app and never be delivered.
                page.locator('#trigger').dispatch_event('keydown', {'key': 'Enter', 'code': 'Enter'})
                page.evaluate("window.dispatchEvent(new Event('blur'))")
                page.locator('#trigger').hover()
                page.mouse.down()
                assert page.locator('#throttle-value').inner_text() == '100%'
                page.mouse.up()
                assert page.locator('#throttle-value').inner_text() == '0%'
                page.get_by_role('button', name='Ion drive', exact=True).click()
                page.locator('#trigger').focus()
                page.keyboard.down('Space')
                page.wait_for_timeout(1500)
                page.wait_for_function("parseFloat(document.querySelector('#output-level').textContent) > -80")
                page.keyboard.up('Space')
                page.screenshot(path=str(OUT / 'toy-desktop.png'), full_page=True)
                page.set_viewport_size({'width': 390, 'height': 844})
                assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
                page.screenshot(path=str(OUT / 'toy-mobile.png'), full_page=True)
                page.get_by_role('button', name='Tune', exact=True).click()
                page.wait_for_function("document.body.dataset.view === 'tune'")
                assert page.evaluate("window.qaContext.state === 'closed'")
                assert page.locator('#designer-title').is_visible()
                page.get_by_role('button', name='Play', exact=True).click()
                assert page.locator('#gear-readout').inner_text() == 'N'
                assert not errors, errors
                report['errors'] = errors
                # Verify the toy also drives the actual native callback fallback.
                fallback = browser.new_page()
                fallback.add_init_script("AudioWorklet.prototype.addModule=async()=>{throw new Error('QA worklet unavailable');};")
                fallback.goto(url)
                fallback.get_by_role('button', name='Start engine', exact=True).click()
                fallback.wait_for_function("document.querySelector('#state-label').textContent === 'Running'")
                fallback.locator('#trigger').focus()
                fallback.keyboard.down('Space')
                fallback.wait_for_function("Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) > 5000 && parseFloat(document.querySelector('#output-level').textContent) > -80")
                report['fallback_output'] = fallback.locator('#output-level').inner_text()
                fallback.keyboard.up('Space')
                fallback.get_by_role('button', name='Heavy diesel', exact=True).click()
                fallback.wait_for_function("Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) < 3500")
                fallback.locator('#trigger').focus()
                fallback.keyboard.down('Space')
                fallback.wait_for_function("Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) > 3000 && parseFloat(document.querySelector('#output-level').textContent) > -80")
                fallback.keyboard.up('Space')
                fallback.get_by_role('button', name='Stop engine', exact=True).click()
                fallback.wait_for_function("document.body.dataset.running === 'false'")
                fallback.close()
                report['passed'] = True
            finally:
                browser.close()
    finally:
        server.terminate()
        server.wait(timeout=5)
        report['server_stopped'] = server.poll() is not None
        (OUT / 'toy-report.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
