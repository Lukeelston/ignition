# Ignition by Elston Works

An engine-sound workbench alongside Syrinx Studio. Build a combustion engine, a turbine, or something that has never passed an emissions inspection.

## Open it

Double-click **Launch Ignition.cmd**. It starts a local server and opens the bench in Chrome. Requires Python 3.11+ and Chrome; there are no packages to install. Both are already present on Luke's workstation.

The normal address is `http://127.0.0.1:8767`. The server only listens on this computer. Use **Quit app** in the footer or Ctrl+C in the launcher console when finished. Closing only the browser window leaves the local server running; the console makes that visible.

From a terminal:

```powershell
python serve.py
```

If port 8767 is in use, use `python serve.py --port 0` and open the printed address. Browser storage is tied to the port, so save your patch as a file before changing ports.

## Use the bench

- Pick **Big-block V8, Straight six, Track four, V-twin, Heavy diesel, Jet turbine, Ion drive**, or **Deep-space hauler**.
- **Start engine** enables live audio. Target RPM and engine load are independent. **Hold to rev** raises both until released; **Rev once** performs a four-second rev. Space or Enter also works while the rev button is focused.
- Shape the exhaust, body, rasp, air, gear whine, spool, roughness, sub rumble, reactor field and metallic resonance. Any layer can be mixed into any powerplant. Cylinder and cycle controls govern the combustion layer; other layers follow shaft speed.
- **Monitor** and **Mute** affect live listening. Exported WAVs retain the synthesizer's full level.
- **Save patch** downloads a versioned JSON file; **Load patch** restores it. Your last settings also persist in this browser at the same local address. The seed reproduces offline renders.
- Choose **Steady RPM**, **Rev up & release**, **Idle to redline**, or **Through the gears**, and a duration of **2–30 seconds**. **Render WAV** downloads **48 kHz, 24-bit, mono PCM** and creates an audition player. Playing the rendered result stops the live engine.

These are synthesized sound-design starting points, not recordings of specific vehicles or a thermodynamic simulation. Cross-bank and uneven firing are expressive approximations. Clips have short edge fades; steady clips are not guaranteed seamless loops. The gear trajectory suggests acceleration, not a gearbox/vehicle physics model.

## Files and verification

- `web/engine.js`: deterministic DSP, presets, patch format, motion curves and WAV encoder.
- `web/worklet.js`: low-latency live synthesis using [AudioWorklet](https://developer.mozilla.org/en-US/docs/Web/API/AudioWorklet).
- `web/render-worker.js`: offline render using the same DSP, away from UI interaction.
- `serve.py`: Python standard-library local server and Chrome launcher.
- `tools/check_browser.py`: actual Chrome integration checks (requires Playwright for development only).
- `tools/render_samples.mjs`: regenerate the V8 and ion-drive listening samples.
- `docs/verification.md`: measured results and limitations.

```powershell
node --test tests/engine.test.mjs
python tools/check_browser.py
node tools/render_samples.mjs
```

No audio samples, remote fonts, network services, account, runtime npm install, or Python package installation are required to use Ignition.
