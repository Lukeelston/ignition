"""Run the native C++ core on the PC. This is NOT a hardware/audio-device test."""
from pathlib import Path
import json
import subprocess
import wave

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'firmware' / 'build'
BUILD.mkdir(parents=True, exist_ok=True)
OUT = ROOT / 'evidence'
OUT.mkdir(exist_ok=True)

def linux(path):
    return subprocess.check_output(['wsl.exe', '--exec', 'wslpath', '-a', str(path)], text=True).strip()

source = linux(ROOT / 'tests' / 'firmware_core.cpp')
binary = linux(BUILD / 'core-check')
subprocess.run(['wsl.exe', '--exec', 'g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', source, '-o', binary], check=True)
result = subprocess.run(['wsl.exe', '--exec', binary, linux(OUT)], capture_output=True, text=True)
if result.returncode:
    print(result.stdout + result.stderr)
    raise SystemExit(result.returncode)
report = json.loads(result.stdout)
parity = subprocess.run(['node', str(ROOT / 'tools' / 'check_firmware_parity.mjs')], check=True, capture_output=True, text=True)
report['parity'] = json.loads(parity.stdout)
with wave.open(str(OUT / 'firmware-v8-host.wav'), 'wb') as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(32000)
    wav.writeframes((OUT / 'firmware-v8.pcm').read_bytes())
report['hardware_verified'] = False
(OUT / 'firmware-host-report.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
