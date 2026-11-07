"""Bounded telemetry from an exact native-USB S3; optional tone or live engine gain."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import time

import serial
from serial.tools import list_ports


def parse_telemetry(line):
    if not line.startswith('preset='):
        return None
    fields = dict(re.findall(r'(\w+)=([^=]*?)(?=\s+\w+=|$)', line))
    required = {'preset', 'gear', 'rpm', 'speed', 'throttle', 'adc', 'blocks',
                'render_max_us', 'render_late', 'write_errors', 'heap'}
    if not required.issubset(fields):
        return None
    try:
        return {key: value.strip() if key == 'preset' else float(value)
                for key, value in fields.items()}
    except ValueError:
        return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', required=True, help='Exact USB serial/MAC of the toy board')
    parser.add_argument('--seconds', type=float, default=8)
    parser.add_argument('--live', action='store_true', help='Print controls as each sample arrives')
    parser.add_argument('--tone', action='store_true', help='Stop engine and request eight quiet diagnostic beeps')
    parser.add_argument('--gain', type=float, choices=[i/10 for i in range(9)], help='Set live engine gain (0 through 0.8); does not start engine')
    args = parser.parse_args()
    if not 1 <= args.seconds <= 180:
        parser.error('--seconds must be 1 through 180')
    matches = [p for p in list_ports.comports()
               if p.vid == 0x303A and p.pid == 0x1001
               and (p.serial_number or '').casefold() == args.serial.casefold()]
    if len(matches) != 1:
        raise SystemExit(f'Expected one matching native-USB S3; found {len(matches)}. No port opened.')
    port = matches[0]
    reader = serial.Serial(port=None, baudrate=115200, timeout=.25, write_timeout=.5)
    reader.dtr = False
    reader.rts = False
    reader.port = port.device
    records = []
    pending = b''
    try:
        reader.open()
        command = (str(round(args.gain*10)).encode('ascii') if args.gain is not None else b'') + (b't' if args.tone else b'')
        if command:
            if reader.write(command) != len(command):
                raise RuntimeError('Diagnostic command was not written completely')
        started = time.monotonic()
        deadline = started + args.seconds
        if args.live:
            print(f'Monitor open: {port.device}, serial {port.serial_number}', flush=True)
        while time.monotonic() < deadline:
            pending += reader.read(min(max(reader.in_waiting, 1), 4096))
            while b'\n' in pending:
                line, pending = pending.split(b'\n', 1)
                sample = parse_telemetry(line.decode('utf-8', errors='replace').strip())
                if sample is not None:
                    records.append(sample)
                    if args.live:
                        controls = ' '.join(f'{key}={sample.get(key, "?")}'
                                            for key in ['state', 'adc', 'throttle', 'rpm', 'volume', 'pcm_peak', 'pcm_rms', 'tone', 'write_errors'])
                        print(f't={time.monotonic()-started:.1f}s {controls}', flush=True)
            if len(pending) > 4096:
                pending = b''
    finally:
        reader.close()
    evidence = Path(__file__).resolve().parents[1] / 'evidence'
    evidence.mkdir(exist_ok=True)
    timestamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    destination = evidence / f'board-{timestamp}.json'
    result = {'port': port.device, 'serial': port.serial_number,
              'seconds': args.seconds, 'tone_requested': args.tone, 'gain_requested': args.gain, 'records': records, 'serial_closed': True}
    destination.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    summary = {key: value for key, value in result.items() if key != 'records'}
    summary.update(samples=len(records), evidence=str(destination))
    if records:
        summary.update(first=records[0], last=records[-1],
                       adc_range=[min(r['adc'] for r in records), max(r['adc'] for r in records)],
                       rpm_range=[min(r['rpm'] for r in records), max(r['rpm'] for r in records)],
                       audio_blocks_advanced=records[-1]['blocks'] > records[0]['blocks'])
    print(json.dumps(summary, indent=2))
    return 0 if len(records) >= 2 else 1


if __name__ == '__main__':
    raise SystemExit(main())
