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

## ESP32-S3 firmware — 2026-11-06, after board photo

The middle board's visible ESP32-S3-WROOM-1 / Lonely Binary Gold markings establish the S3 target. Exact memory capacity and USB identity are not yet probed. The firmware uses no PSRAM and a conservative 4 MB flash layout. Luke authorized borrowing Iris's audio assembly. The chat **Design Iris’s embodied form** was consulted and the current Iris hardware document and speaker source were read: HW-792/MAX98357A, measured approximately 4 Ω donor speaker, 39 × 39 × 22 mm, GPIO15/16/17 and 32-bit stereo slots. No Iris files, runtime, wiring or firmware were changed by tools.

`python tools/build_firmware.py`: **passes** using Arduino CLI 1.5.1 and ESP32 core 3.3.12, target ESP32-S3 at 240 MHz. Final build: **345,214 bytes** of program storage and **34,600 bytes** of static RAM. The latter excludes runtime driver allocations and task stacks. Binary and compile log are in `firmware/build/`. No upload or COM-port access occurred.

`python tools/check_firmware.py`: **passes** with real native C++ control and DSP code under host g++. All 8 patches produce finite, bounded, non-silent samples and fade to exact silence. Aggregate measured normalized RMS 0.08579, peak 0.24406 at the harness gain; synth object 11,904 bytes on the host ABI. Checks cover ignition boot interlock, rejection with held throttle, switch debounce, one shift per held button, gearing/boost/brake, over-rev refusal, firing frequency, signed PCM packing and determinism. A 20-second native drive trace matches the browser model at 20 checkpoints, maximum observed RPM difference **0.005786**. Host WAV is `evidence/firmware-v8-host.wav`; it is not an ESP capture.

Review found stale synth state after a stop/restart. The added regression failed before the fix and passes after resetting DSP state on a new audible gate. It compares all 6,400 samples (200 ms) after a high-RPM shutdown/restart against a fresh cranking synth, then repeats after changing to the ion patch while stopped. This includes the 864-sample reflection buffer. Review confirmed the fix and found no additional actionable source issues. All 19 existing Node tests also pass.

Remaining physical proof: connected-board identity and backup, flash, calibrated trigger, I²S waveform/speaker capture, acoustic quality, realtime render deadlines, input behavior, shutdown and comfortable volume. **Hardware is unverified — needs user confirmation.** The firmware reports timing and I²S errors but has not run on the photographed board.

## Thumbstick input profile — 2026-11-06

Replaced the full-travel trigger mapping with calibrated centre-to-endpoint travel. The new native tests initially failed to compile because the mapping function was absent, then passed after implementation. Checks cover idle across centre jitter, the entire opposite direction, midpoint/full travel, both endpoint directions, saturation, invalid calibration, and full-travel pot compatibility. A controller sequence starts from a centred stick, revs in neutral, returns to idle and refuses ignition with the stick held forward. The existing firmware DSP and 20-checkpoint browser parity checks still pass; all 19 Node tests pass.

`python tools/build_firmware.py` passes for the existing S3 target/core, using **345,234 bytes** program storage and **34,600 bytes** static RAM. Independent source review found no actionable issues. `throttleRest=2048`, `throttleFull=3900`, and `throttleDeadZone=300` are provisional raw ADC counts, not measurements of Luke's module. GPIO4 is the stick axis and GPIO7 its optional brake click; all other assignments are preserved. No COM port was opened and no upload occurred. Actual centre, travel direction, sound and realtime board behavior are **unverified — needs user confirmation**.

## First physical flash and USB telemetry — 2026-11-06 (America/Chicago)

After Luke confirmed wiring and the connected board, live enumeration and `esptool flash-id` identified **COM9, 58:e6:c5:68:65:c8, ESP32-S3 rev 0.2, 16 MB flash, 8 MB embedded PSRAM**. COM7's identity matches the separate printer display and was not opened; COM6 is the printer and was not opened. Iris's documented USB identity is different. No Iris runtime or firmware was changed.

Saved all **16,777,216 bytes** of pre-existing toy flash to `firmware/build/backups/58e6c56865c8-before-ignition-20261106.bin`; verified its digest against the device before writing. Backup SHA256: `c2bf0e0d9c2acebb5aa89540495242216f05d2b777809f307936127e9127e096`. This private recovery image is ignored by Git and excluded from release packaging.

Enabled hardware USB CDC at boot and mirrored diagnostics to both USB sockets. USB writes have zero timeout and require available buffer capacity, so stalled host reads cannot hold the input loop. Target compilation passes: **349,730 bytes** program storage, **34,792 bytes** static RAM. The native firmware/DSP/parity checks pass. Independent source review checked the installed HWCDC implementation and found no actionable issue. No changes to the global Arduino installation were made.

Flashed the conservative 4 MB merged image and separately verified its digest against flash. Image SHA256: `6c50c0fdcb45b784524c13229041db29394a5c1691d0a494f9587b629cf90abe`. Operation logs and manifest: `firmware/build/bench-install.json`, `backup-verified.log`, `ignition-written.log`, `ignition-verified.log`. Exact USB identity was checked immediately before each operation.

`python tools/read_board.py --serial 58:e6:c5:68:65:c8 --seconds 8` captured **8 live records** from the running board, exercising the parser against real firmware output including the spaced preset name **Big-block V8**. Evidence: `evidence/board-20261107T044739Z.json` (UTC filename; local date November 6). Released-stick ADC **1895–2077**, throttle **0.00**, neutral and **0 RPM** while off. Audio blocks advanced **1498 → 3248**; `render_max_us=121`, `render_late=0`, `write_errors=0`, stable heap 316,500. This was the silent path and does not benchmark active synthesis or prove sound from the speaker. Serial closed after capture. Physical listening, ignition/rev/release, full travel calibration and the full control sequence remain **unverified — needs user confirmation**.

A subsequent **120-second, 120-record** capture (`evidence/board-20261107T045014Z.json`) stayed at 0 RPM and zero throttle; no successful ignition was observed. ADC ranged **0–2076**, without an observed high endpoint; this alone does not establish which physical direction was moved. Silent audio blocks advanced **12,248 → 41,998**, with zero write errors/late renders and unchanged heap. The user has been asked to perform and report the ignition/rev/listening sequence; no response was received during this capture. Both serial captures closed normally. The packaged source smoke check passed and its server stopped.
