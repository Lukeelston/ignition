"""Compile only; deliberately never selects a serial port or uploads firmware."""
from pathlib import Path
import json
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'firmware' / 'build'
CLI = shutil.which('arduino-cli') or str(Path.home() / 'AppData/Local/Programs/arduino-cli/arduino-cli.exe')
FQBN = 'esp32:esp32:esp32s3:FlashSize=4M,PSRAM=disabled,PartitionScheme=default,USBMode=hwcdc,CDCOnBoot=default,CPUFreq=240,FlashMode=qio'
BUILD.mkdir(parents=True, exist_ok=True)
result = subprocess.run([CLI, 'compile', '--fqbn', FQBN, '--build-path', str(BUILD / 'objects'), '--output-dir', str(BUILD / 'esp32s3'), str(ROOT / 'firmware' / 'ignition')], cwd=ROOT, capture_output=True, text=True, timeout=600)
(BUILD / 'compile.log').write_text(result.stdout + result.stderr, encoding='utf-8')
print(result.stdout)
if result.returncode:
    print(result.stderr)
    raise SystemExit(result.returncode)
print(json.dumps({'compiled': True, 'uploaded': False, 'fqbn': FQBN, 'output': str(BUILD / 'esp32s3')}))
