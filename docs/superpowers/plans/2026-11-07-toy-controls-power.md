# Toy control and power refinement

> Implementation stays in the existing Ignition branch and bench checkout. The user's request authorizes these changes and the identified toy update; no new wiring or service is needed.

**Goal:** louder sound, roughly one-third more pull, smooth climbing revs, remapped axes, and hold-to-cycle engine types.

**Architecture:** retain the native C++ core and matching browser DSP/drive model. Swap GPIO1/4 roles in firmware, extend the existing debounced click controller, and add a bounded output gain stage. No amplifier or I2S changes.

**Decisions:** GPIO1 high = throttle, low = proportional brake. GPIO4 high (old throttle direction) = upshift, low = downshift. Short click toggles on release; hold 650 ms changes character, repeats every 900 ms, release does not toggle ignition. Boot-held click stays ignored until released. A start tap must have idle throttle throughout. Power coefficient +35% is a toy tuning change, not measured horsepower. Target +6 dB signal amplitude with a gentle peak ceiling below PCM clipping; diagnostic tones retain their existing level.

**Verification:** reproduce commanded-RPM versus firing-pitch behavior, then regress it in both synths. Test tap/hold/bounce/boot-held/throttle-start rejection, analog braking and character changes under motion. Measure all eight presets through the louder output stage. Compile and review before flashing, verify exact USB identity and flash digest, then capture actual new controls and ask Luke for sound/feel feedback. Close all test processes and serial readers.

- [x] Reproduce surging; constrain combustion crank flutter to idle, preserving acceleration texture and sci-fi modulation. Node/native tests red then green.
- [x] Remap control pins, add proportional brake and release-based tap/hold semantics. Regress full controller behavior and browser/native drive parity.
- [x] Raise drive torque by 35%; add +6 dB bounded output stage; test all presets/start/stop and arbitrary input bounds.
- [x] Independent integration review, build, exact-device flash/readback and physical control capture. Listening verdict remains pending.
- [x] Refresh guide and standalone app; evidence recorded in `docs/verification.md`.

Closeout: package the release, commit this refinement and verify process cleanup. Hardware control response is verified; acoustic acceptance remains a separate user check.
