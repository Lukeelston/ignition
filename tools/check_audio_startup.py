"""Exercise real audio when module loading fails, and bounded startup errors."""
from pathlib import Path
import json
import subprocess
import sys
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]


def main():
    server = subprocess.Popen([sys.executable, str(ROOT / 'serve.py'), '--no-browser', '--port', '0'], stdout=subprocess.PIPE, text=True)
    results = {}
    try:
        url = json.loads(server.stdout.readline())['url']
        with sync_playwright() as pw:
            browser = pw.chromium.launch(channel='chrome', headless=True)
            try:
                page = browser.new_page()
                # Force the actual dependency boundary to fail; the fallback must
                # still generate real audio through a native AudioContext.
                page.add_init_script("AudioWorklet.prototype.addModule = async () => { throw new DOMException('Module loading unavailable', 'SecurityError'); };")
                page.goto(url)
                page.get_by_role('button', name='Tune', exact=True).click()
                page.get_by_role('button', name='Start engine', exact=True).click()
                page.wait_for_function("document.body.dataset.running === 'true'", timeout=5000)
                page.wait_for_function("Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) > 900 && parseFloat(document.querySelector('#output-level').textContent) > -80", timeout=5000)
                results['fallback_output'] = page.locator('#output-level').inner_text()
                page.locator('#rpm').fill('4200')
                page.locator('#rpm').dispatch_event('input')
                page.wait_for_function("Number(document.querySelector('#rpm-readout').textContent.replace(/,/g,'')) > 3500")
                page.get_by_role('button', name='Stop engine', exact=True).click()
                page.wait_for_function("document.body.dataset.running === 'false'")
                page.close()

                blocked = browser.new_page()
                blocked.add_init_script("AudioContext.prototype.resume = () => new Promise(() => {});")
                blocked.goto(url)
                blocked.get_by_role('button', name='Start engine', exact=True).click()
                blocked.wait_for_function("document.querySelector('#status').dataset.error === 'true'", timeout=6000)
                assert blocked.get_by_role('button', name='Start engine', exact=True).is_enabled()
                assert blocked.locator('body').get_attribute('data-running') == 'false'
                results['blocked_startup'] = blocked.locator('#status').inner_text()
                blocked.close()
            finally:
                browser.close()
    finally:
        server.terminate()
        server.wait(timeout=5)
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
