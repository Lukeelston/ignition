"""Package the runnable source, samples and checks; smoke test the extracted bundle."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'release' / 'Ignition-0.1.zip'
DEST.parent.mkdir(exist_ok=True)
files = [ROOT / name for name in ['README.md', 'serve.py', 'Launch Ignition.cmd', 'package.json']]
for directory in ['web', 'samples', 'tests', 'tools', 'docs']:
    files.extend(p for p in (ROOT / directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(DEST, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in files:
        archive.write(path, Path('Ignition') / path.relative_to(ROOT))

with tempfile.TemporaryDirectory(prefix='ignition-package-check-') as directory:
    with zipfile.ZipFile(DEST) as archive:
        archive.extractall(directory)
    extracted = Path(directory) / 'Ignition'
    server = subprocess.Popen([sys.executable, str(extracted / 'serve.py'), '--no-browser', '--port', '0'], stdout=subprocess.PIPE, text=True)
    try:
        url = json.loads(server.stdout.readline())['url']
        with urllib.request.urlopen(url, timeout=5) as response:
            assert b'<title>Ignition' in response.read()
        with urllib.request.urlopen(url + '/worklet.js', timeout=5) as response:
            assert response.headers.get_content_type() == 'text/javascript'
        request = urllib.request.Request(url + '/__quit', data=b'', method='POST', headers={'Origin': url, 'X-Ignition-Control': 'quit'})
        with urllib.request.urlopen(request, timeout=5) as response:
            assert response.status == 204
        server.wait(timeout=5)
    finally:
        if server.poll() is None:
            server.terminate()
            server.wait(timeout=5)
print(json.dumps({'bundle': str(DEST), 'files': len(files), 'bytes': DEST.stat().st_size, 'extracted_app_smoke': 'passed', 'server_stopped': True}))
