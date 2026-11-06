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
