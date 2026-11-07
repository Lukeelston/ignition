# Ignition by Elston Works

A handheld engine toy and sound-design bench alongside Syrinx Studio. Squeeze a trigger, shift gears and flick boost, using combustion engines, turbines or something that has never passed an emissions inspection.

## Open it

Double-click **Ignition.html** and open it in Chrome. This self-contained edition includes the UI, synthesizer, worklet and render worker. It needs no server, Python, packages, or adjacent files. Opening `web/index.html` directly forwards to this edition.

**Launch Ignition.cmd** is the alternative server launcher. It starts a local server and opens the modular edition in Chrome. This alternative requires Python 3.11+; there are no packages to install. Python and Chrome are already present on Luke's workstation.

The server edition's normal address is `http://127.0.0.1:8767`. It only listens on this computer. Use **Quit app** in the footer or Ctrl+C in the launcher console when finished. Closing only that edition's browser window leaves the local server running. The standalone HTML has no server to leave behind; Quit app stops its audio, and you can close the tab.

From a terminal:

```powershell
python serve.py
```

If port 8767 is in use, use `python serve.py --port 0` and open the printed address. Browser storage is tied to the port, so save your patch as a file before changing ports.

## Play

Ignition opens in **Play**, the browser prototype of a handheld toy. **Start engine**, wait for it to catch, tap **gear +** for first, then hold **Squeeze trigger**. The throttle slider gives partial throttle and springs back on release. Six gears plus neutral; an automatic clutch handles launching. Shifting up drops RPM, braking slows virtual road speed, and the boost switch increases acceleration. Downshifts that would over-rev are refused.

**Space** operates the focused trigger; **↑ / ↓** shift and **B** brakes while focus is outside a form input. Mouse and touch controls work too. Releasing, cancelling a pointer, or losing window focus releases momentary controls. Hiding the tab turns toy audio off. **Tune** opens the existing sound-design tools; switching views stops the engine and preserves the patch. Play controls RPM and load dynamically, without overwriting the saved bench settings.

[Handheld layout and ESP implementation map](docs/handheld.md) records the physical controls and port plan. Board identification is awaiting Luke's photo. This release contains the browser toy, **not ESP firmware**.

## Use the bench (Tune)

- Pick **Big-block V8, Straight six, Track four, V-twin, Heavy diesel, Jet turbine, Ion drive**, or **Deep-space hauler**.
- **Start engine** enables live audio. Target RPM and engine load are independent. **Hold to rev** raises both until released; **Rev once** performs a four-second rev. Space or Enter also works while the rev button is focused.
- Shape the exhaust, body, rasp, air, gear whine, spool, roughness, sub rumble, reactor field and metallic resonance. Any layer can be mixed into any powerplant. Cylinder and cycle controls govern the combustion layer; other layers follow shaft speed.
- **Monitor** and **Mute** affect live listening. Exported WAVs retain the synthesizer's full level.
- **Save patch** downloads a versioned JSON file; **Load patch** restores it. Your last settings also persist in this browser at the same local address. The seed reproduces offline renders.
- Choose **Steady RPM**, **Rev up & release**, **Idle to redline**, or **Through the gears**, and a duration of **2–30 seconds**. **Render WAV** downloads **48 kHz, 24-bit, mono PCM** and creates an audition player. Playing the rendered result stops the live engine.

These are synthesized sound-design starting points, not recordings of specific vehicles or a thermodynamic simulation. Cross-bank and uneven firing are expressive approximations. Clips have short edge fades; steady clips are not guaranteed seamless loops. The WAV export's gear trajectory is a scripted sweep, separate from Play's interactive simplified driving model.

## Files and verification

- `web/engine.js`: deterministic DSP, presets, patch format, motion curves and WAV encoder.
- `web/drive.js`: browser-independent ignition, throttle, gearing, virtual speed, boost and brake controller.
- `tools/check_toy.py`: real Chrome interaction checks and live audio capture, including callback fallback.
- `web/worklet.js`: low-latency live synthesis using [AudioWorklet](https://developer.mozilla.org/en-US/docs/Web/API/AudioWorklet).
- Direct-file contexts and failed worklet loads use the same DSP through a native audio callback. Audio is resumed immediately on the click; startup failures time out and appear beside the ignition button.
- `web/render-worker.js`: offline render using the same DSP, away from UI interaction.
- `serve.py`: Python standard-library local server and Chrome launcher.
- `tools/check_browser.py`: actual Chrome integration checks (requires Playwright for development only).
- `tools/build_standalone.py`: rebuild `Ignition.html` after changing any web source.
- `tools/check_standalone.py`: run the standalone bundle in Chrome with no adjacent assets served; checks live audio, WAV export and shutdown.
- `tools/check_audio_startup.py`: exercise actual audio with worklet loading rejected, plus recovery from an audio-resume promise that never resolves.
- `tools/render_samples.mjs`: regenerate the V8 and ion-drive listening samples.
- `docs/verification.md`: measured results and limitations.

```powershell
node --test tests/engine.test.mjs
python tools/check_browser.py
python tools/build_standalone.py
python tools/check_standalone.py
python tools/check_audio_startup.py
node tools/render_samples.mjs
```

No audio samples, remote fonts, network services, account, runtime npm install, or Python package installation are required to use Ignition.
