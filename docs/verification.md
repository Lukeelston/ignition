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

## Direct-opening fix

User reported that Start engine did nothing. Chrome tab inventory showed `file:///C:/Users/Luke/Projects/engine-generator/web/index.html`, and no local server was running. The original HTML loaded external ES modules, which Chrome blocks from file origins. The earlier integration test exercised only the HTTP launcher path and missed this delivery failure.

Added a self-contained `Ignition.html` with inline UI/DSP and Blob-backed worklet/render-worker code. The original index redirects direct-file opens to it. Standalone Quit stops audio without requesting a nonexistent server. No browser security settings are changed.

`python tools/check_standalone.py` passed using the generated HTML served alone over loopback: eight presets, real live output, a valid two-second 24-bit WAV download, and clean Quit. No adjacent scripts, stylesheets, or other runtime assets were requested. Direct `file:` inspection is unavailable through the browser tool; direct-open behavior is awaiting the user's confirmation rather than being reported as automated proof.

## Follow-up: zero RPM in embedded file preview

The standalone file was still reported inert in the in-app browser. Opened the actual in-app browser on the HTTP edition, pressed Start engine, and observed 950 RPM and about -28.5 dBFS. After startup hardening, repeated with the user's saved patch: **1,190 RPM and about -21.3 dBFS at 35% monitor volume**. This is direct UI evidence in the affected browser product, not only a separate headless Chrome test. The file-origin exception itself remains unavailable for inspection; its exact message was not obtained.

The direct-file path now selects callback audio with the same EngineSynth and no worklet-module fetch. HTTP uses AudioWorklet first, with the callback as fallback if loading is unavailable or stalls. Audio resume starts synchronously from the click, has a 2.5-second deadline, and reports failures next to the control. A failed resume leaves Start engine enabled.

`check_audio_startup.py` first failed with worklet loading rejected, then passed after the fallback: real audio output and RPM changes, stop, and bounded recovery from a never-resolving resume. The 13 DSP tests, full modular Chrome checks, and isolated standalone checks also passed after these changes. Hearing the user's physical speakers still requires user confirmation.

## Handheld toy follow-up — 2026-11-06

19 Node checks pass: the previous 13 synthesis/patch/render tests plus 6 tests for ignition, neutral, acceleration, gear changes, braking, boost, gear limits, over-rev refusal and live configuration.

`tools/check_toy.py` drives actual Chrome controls. Captured first-to-second shift: 3,711 → 3,185 RPM while throttle remains held. The two-second live worklet capture (`evidence/toy-live-capture.wav`) has RMS 0.253 and peak 0.633. Callback audio with deliberately failed worklet loading produced -26.2 dBFS in the recorded run. Tests exercise partial throttle release, pointer cancellation, missing keyup/window blur followed by a new pointer click, brake, boost, combustion/ion switching, and high-RPM preset changes to a lower redline through callback audio. No page errors. Desktop/mobile screenshots inspected; no horizontal page overflow at 390 px.

The Tune integration check passes patch save/load, live RPM, mute/rev, WAV export, preview switching, restart, persistence and clean shutdown. Switching from a playing rendered preview to Play now stops the hidden preview. Audio-startup regression checks still pass for failed worklet loading and a never-resolving resume promise. The rebuilt self-contained artifact passes its loopback test with no adjacent assets served.

Review found and corrected stale keyboard-hold flags, preset/audio configuration ordering, and hidden preview playback. Test browsers and temporary servers were shut down. ESP firmware, GPIO assignments and physical audio remain pending the board photo and bench testing. The browser control model is JavaScript; it has not yet been ported or benchmarked on an ESP. Direct-file embedded-browser behavior remains **unverified — needs user confirmation**; automated verification uses loopback HTTP.
