"""Test the portable HTML with no adjacent assets or network dependencies.

Served over loopback for browser automation; direct file-tab confirmation is manual.
"""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json
import threading
import wave
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]


def main():
    artifact = ROOT / 'Ignition.html'
    assert artifact.is_file(), 'A self-contained Ignition.html must exist for direct opening'
    payload = artifact.read_bytes()
    requests = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def do_GET(self):
            requests.append(self.path)
            if self.path == '/':
                self.send_response(200)
                self.send_header('Content-Type', 'text/html; charset=utf-8')
                self.end_headers()
                self.wfile.write(payload)
            else:
                self.send_response(404)
                self.end_headers()

    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    thread = threading.Thread(target=server.serve_forever)
    thread.start()
    result = {}
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(channel='chrome', headless=True)
            try:
                page = browser.new_page()
                errors = []
                page.on('pageerror', lambda error: errors.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}')
                assert page.locator('#presets button').count() == 8
                page.get_by_role('button', name='Start engine', exact=True).click()
                page.wait_for_function("document.body.dataset.running === 'true'", timeout=5000)
                page.wait_for_function("parseFloat(document.querySelector('#output-level').textContent) > -80", timeout=5000)
                result['live_output'] = page.locator('#output-level').inner_text()
                page.locator('#duration').fill('2')
                with page.expect_download(timeout=10000) as download:
                    page.get_by_role('button', name='Render WAV', exact=True).click()
                path = ROOT / 'evidence' / 'standalone-render.wav'
                download.value.save_as(path)
                with wave.open(str(path)) as wav:
                    assert (wav.getframerate(), wav.getsampwidth(), wav.getnframes()) == (48000, 3, 96000)
                page.get_by_role('button', name='Quit app', exact=True).click()
                page.wait_for_function("document.body.dataset.running === 'false'")
                assert page.locator('#status').inner_text() == 'Ignition is closed. You can close this window.'
                assert not errors, errors
                assert all(path in ['/', '/favicon.ico'] for path in requests), requests
                result.update(passed=True, network_assets_requested=False, errors=errors)
            finally:
                browser.close()
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)
        assert not thread.is_alive()
    (ROOT / 'evidence' / 'standalone-report.json').write_text(json.dumps(result, indent=2))
    print(json.dumps(result))


if __name__ == '__main__':
    main()
