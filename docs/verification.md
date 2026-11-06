# Ignition verification — 2026-11-06

## Passed

- `node --test tests/engine.test.mjs`: **13/13**. Tests cover every preset, deterministic seeds, finite/bounded output at extreme settings, real firing counts for 2-/4-stroke engines, RPM response, load response, live seed changes, motion curves, duration limits, imported-patch validation and exact 24-bit PCM bytes.
- `python tools/check_browser.py`: **passed in installed Google Chrome**, no page errors. Starts the real AudioWorklet, captures a signal (peak 0.28015, RMS 0.12428 at the initial V8 setting), and records one second to `evidence/live-worklet-capture.wav`.
- Actual UI interactions: target RPM, keyboard rev/release, pointer cancellation, mute/unmute, JSON save/load, last-patch restoration on reload, worker-rendered WAV download, preview/live audition exclusivity, stop/restart, and Quit app.
- Downloaded browser WAV: **48,000 Hz / 24-bit / mono / 96,000 frames** for the 2-second test.
- Desktop 1440px and mobile 390px screenshots inspected; mobile has no horizontal overflow.
- Review findings reproduced before fixes: live seed ignored; imported RPM above redline; starting live audio over an active preview. Each regression now passes. No deferred review findings.
- V8 and ion samples: 8 seconds each, peaks 0.56516 and 0.50608 respectively, finite and below clipping.

## Limits

These checks establish generated audio, a functioning browser signal path, and usable files. They do not establish fidelity to real engines or sound from Luke's physical speakers. **Perceived realism and physical output: unverified — needs user confirmation.**

This is procedural sound design with simplified firing and resonance models. No seamless-loop guarantee, real vehicle simulation, multichannel spatialization, or external MIDI/game-engine control is claimed.

Browser QA uses a separate headless Chrome profile. Its browser and local server are closed in a finally block; no background service is installed. The user launcher intentionally leaves its visible server console running until Quit app or Ctrl+C.
