# Ignition execution record

2026-11-06. User scope: companion engine-sound program to Syrinx; combustion and sci-fi with extensive sound control.

Builder choices: standalone sibling repo, provisional name Ignition, dependency-free browser DSP and Python local server; local-only application rather than hosted site. All work is reversible and within requested scope. Branch `ignition` in a newly initialized repo. Syrinx unchanged.

- Task 1: created core behavior tests, observed missing-synth assertion fail, implemented shared synthesis and WAV export, 11/11 passed. Commit 0521a61.
- Task 2: created real Chrome harness, observed missing UI failure, implemented bench. Harness then exposed missing startup patch restoration. Reload regression failed, restoration fixed, full browser flow passed. Commit 3fa966f.
- Stable default port 8767 chosen so browser patch persistence survives normal relaunch; test harness explicitly uses ephemeral ports.
- Desktop/mobile screenshots inspected; dark-bench label contrast corrected.
- Final review requested via requesting-code-review skill; listening samples and delivery documentation prepared in parallel.

Audio validation establishes a real browser signal and valid exports, not subjective realism or sound from the user's physical speakers.

- Final review found two important issues (live seed ignored, imported RPM over redline) and an audition-overlap defect. Added regression tests, observed all three failures, implemented fixes, then core suite 13/13 and full Chrome flow passed.
- One-second real AudioWorklet capture retained alongside browser WAV and screenshots. Mute and pointer cancellation exercised as additional lifecycle checks.
- No deferred findings. The audition overlap was treated as a functional issue because mixing two engines prevents useful comparison; it was fixed in the same verified pass.

## Follow-up: Start engine did nothing

User opened the raw web/index.html on disk; tab inventory confirmed a file URL and no server listener. External ES module loading from that context was the failure. Added a generated standalone HTML and a file-entry redirect; same source synthesizer, Blob modules, and no server required. New regression began with the missing-standalone assertion failing, then passed live-audio/render/shutdown checks with the artifact served alone and all asset requests unavailable. Existing DSP and full modular-browser regression suites rerun. Requested direct-file confirmation from Luke because the browser tool cannot inspect file URLs.

## Follow-up: file preview still remained at zero RPM

Reproduced a working HTTP launch in the actual in-app browser (950 RPM, nonzero meter), retaining the file-origin exception as unknown rather than claiming to have inspected it. Added a deterministic fallback for file contexts/failed worklet loading, immediate user-gesture resume, bounded startup, and visible error location. Regression with rejected worklet load failed first, passed after implementation; stuck resume also returns to an enabled Start button. All existing checks passed. Refreshed the live in-app tab, preserved the user's patch (1,190 RPM / 68% load) and monitor setting (35%), observed running at 1,190 RPM and -21.3 dBFS. Asked whether the user hears it and wants the intentional local server left running, in light of the explicit cleanup rule.

## Handheld direction — 2026-11-06

Luke clarified that Ignition should be a toy, potentially ESP-powered, with triggers, switches and gears. He chose handheld controls and will supply a board photo. Added a Play-first browser prototype: spring-return throttle, six gears/neutral, automatic clutch, brake and boost; kept sound design under Tune. `ToyDrive` separates interactive motion from audio synthesis and saved patch settings. The physical interaction layout and a sourced ESP port map are in `docs/handheld.md`; pins remain unspecified and no existing device was reflashed. Browser output, input-release behavior and original tuning functions were verified; physical hardware awaits identification and testing.

## S3 photo and borrowed Iris audio — 2026-11-06

Luke supplied two matching-layout Lonely Binary boards and a classic ESP32 in a photo; the middle module clearly says ESP32-S3-WROOM-1. Selected that S3 profile without inferring exact memory capacity. Luke authorized temporarily borrowing Iris's amp/speaker and directed us to her physical-embodiment chat. Read **Design Iris’s embodied form**, then verified its hardware references against the current Iris files. Preserved GPIO15/16/17 and 32-bit stereo slots for the HW-792/MAX98357A. The driver/speaker can move as an assembly; Iris software remains untouched.

Implemented native C++ drive/controller/synth, 100 Hz debounced controls, lookup oscillators/saturation, a separate 32 kHz I²S task, input-staleness mute, boot interlock, and onboard RGB plus UART telemetry. All eight browser patches can be regenerated into a firmware header; an optional saved custom patch adds a ninth character. Added compile-only tooling, host tests, drive parity verification and a complete wiring/bring-up guide. ESP32-S3 compilation and host checks pass; board identity, flash, physical audio and control timing remain pending the connected spare board. Review found and corrected stale DSP state on ignition restart. No device was flashed or reset.

## Thumbstick bench controls — 2026-11-06

Luke reported the speaker connected and requested a staged build walkthrough using available buttons, switches and rotary encoders, then found a thumbstick. Chose the spring-centred stick as the throttle and its click as the existing brake input. Replaced full-travel trigger normalization with centre-to-endpoint normalization, a 300-count provisional centre dead zone, and either endpoint direction. The old mapping would have produced approximately half throttle at centre and rejected ignition. No automatic boot calibration; measured raw rest/full values are editable in the board profile. Kept all GPIO assignments and audio wiring. Added software regressions, compiled the target, and wrote staged wiring/first-sound/calibration instructions. Thumbstick labels, physical calibration, board upload and playback still require the bench session.
