"""Ignition's dependency-free local launcher. Ctrl+C or Quit app closes the server."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse
import json
import os
import subprocess
import threading

ROOT = Path(__file__).resolve().parent


class Handler(SimpleHTTPRequestHandler):
    extensions_map = {**SimpleHTTPRequestHandler.extensions_map, '.js': 'text/javascript'}

    def log_message(self, *_args):
        pass

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        super().end_headers()

    def do_GET(self):
        if self.path == '/favicon.ico':
            self.send_response(204)
            self.end_headers()
            return
        super().do_GET()

    def do_POST(self):
        origin = f'http://127.0.0.1:{self.server.server_port}'
        if self.path != '/__quit' or self.headers.get('Origin') != origin or self.headers.get('X-Ignition-Control') != 'quit':
            self.send_error(403)
            return
        self.send_response(204)
        self.end_headers()
        threading.Thread(target=self.server.shutdown, daemon=True).start()


def make_server(port=0):
    return ThreadingHTTPServer(('127.0.0.1', port), partial(Handler, directory=str(ROOT / 'web')))


def find_chrome():
    candidates = [Path(os.environ.get(base, '')) / suffix for base, suffix in [
        ('PROGRAMFILES', 'Google/Chrome/Application/chrome.exe'),
        ('PROGRAMFILES(X86)', 'Google/Chrome/Application/chrome.exe'),
        ('LOCALAPPDATA', 'Google/Chrome/Application/chrome.exe'),
    ]]
    return next((p for p in candidates if p.is_file()), None)


def main():
    parser = argparse.ArgumentParser(description='Open the Ignition engine-sound workbench.')
    parser.add_argument('--port', type=int, default=8767, help='Default 8767 keeps browser patch storage stable; 0 selects a free port')
    parser.add_argument('--no-browser', action='store_true')
    args = parser.parse_args()
    server = make_server(args.port)
    url = f'http://127.0.0.1:{server.server_port}'
    print(json.dumps({'url': url, 'pid': os.getpid()}), flush=True)
    print('Ignition is local. Use Quit app in the bench, or Ctrl+C here, to stop it.', flush=True)
    if not args.no_browser:
        chrome = find_chrome()
        if chrome:
            subprocess.Popen([str(chrome), f'--app={url}', '--new-window'])
        else:
            print(f'Chrome was not found. Open this address in Chrome: {url}', flush=True)
    try:
        server.serve_forever(poll_interval=.2)
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
