"""Bundle our controlled ES modules into one HTML; no external runtime fetches."""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / 'web'
LOCAL_ENTRY = "  <script data-local-entry>if(location.protocol==='file:')location.replace('../Ignition.html');</script>\n"
ASSETS = '  <link rel="stylesheet" href="style.css"><script type="module" src="app.js"></script>'


def inline_module(name):
    lines = []
    for line in (WEB / name).read_text(encoding='utf-8').splitlines():
        if line.startswith('import '):
            if not line.endswith(';'):
                raise ValueError(f'Bundler requires a single-line import in {name}')
            continue
        lines.append(line.removeprefix('export '))
    return '\n'.join(lines)


def build():
    html = (WEB / 'index.html').read_text(encoding='utf-8')
    assert html.count(ASSETS) == 1 and html.count(LOCAL_ENTRY) == 1
    html = html.replace(LOCAL_ENTRY, '').replace(ASSETS, '<style>' + (WEB / 'style.css').read_text(encoding='utf-8') + '</style>')
    core = inline_module('engine.js')
    bundle = {'worklet.js': core + '\n' + inline_module('worklet.js'), 'render-worker.js': core + '\n' + inline_module('render-worker.js')}
    # Escape HTML delimiters inside JS strings before embedding the worklet sources.
    configuration = json.dumps(bundle, ensure_ascii=True).replace('<', '\\u003c')
    script = f'window.IGNITION_BUNDLE={configuration};\n' + core + '\n' + inline_module('drive.js') + '\n' + inline_module('app.js')
    html = html.replace('</body>', '<script>\n' + script.replace('</script', '<\\/script') + '\n</script>\n</body>')
    output = ROOT / 'Ignition.html'
    output.write_text(html, encoding='utf-8')
    return output


if __name__ == '__main__':
    print(build())
