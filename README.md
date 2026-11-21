# Ignition by Elston Works

A handheld engine toy and sound-design bench alongside Syrinx Studio. Squeeze a trigger, shift gears and flick boost, using combustion engines, turbines or something that has never passed an emissions inspection.

## Open it

Double-click **Ignition.html** and open it in Chrome. This self-contained edition includes the UI, synthesizer, worklet and render worker. It needs no server, Python, packages, or adjacent files. Opening `web/index.html` directly forwards to this edition.

**Launch Ignition.cmd** is the alternative server launcher. It starts a local server and opens the modular edition in Chrome. This alternative requires Python 3.11+ and Chrome; there are no Python packages to install.

The server edition's normal address is `http://127.0.0.1:8767`. It only listens on this computer. Use **Quit app** in the footer or Ctrl+C in the launcher console when finished. Closing only that edition's browser window leaves the local server running. The standalone HTML has no server to leave behind; Quit app stops its audio, and you can close the tab.

From a terminal:

```powershell
python serve.py
```

If port 8767 is in use, use `python serve.py --port 0` and open the printed address. Browser storage is tied to the port, so save your patch as a file before changing ports.

## Play

Ignition opens in **Play**, the browser prototype of a handheld toy. **Start engine**, wait for it to catch, tap **gear +** for first, then hold **Squeeze trigger**. The throttle slider gives partial throttle and springs back on release. Six gears plus neutral; an automatic clutch handles launching. Shifting up drops RPM, braking slows virtual road speed, and the boost switch increases acceleration. Downshifts that would over-rev are refused.

**Space** operates the focused trigger; **↑ / ↓** shift and **B** brakes while focus is outside a form input. Mouse and touch controls work too. Releasing, cancelling a pointer, or losing window focus releases momentary controls. Hiding the tab turns toy audio off. **Tune** opens the existing sound-design tools; switching views stops the engine and preserves the patch. Play controls RPM and load dynamically, without overwriting the saved bench settings.

[ESP32-S3 firmware and wiring](firmware/README.md) cover the Lonely Binary S3 board and MAX98357A I²S amplifier. The current bench build uses a single spring-centred thumbstick:

| Connection | Action |
| --- | --- |
| VRx → GPIO1 | Right accelerates; left applies proportional brake |
| VRy → GPIO4 | High direction shifts up; opposite shifts down; re-centre between shifts |
| SW → GPIO8 | Short click/release starts or stops; hold cycles engine types |

Hold for 0.65 seconds to change engine, then every 0.9 seconds while held. The firmware includes a crank/catch/idle sequence, six gears plus neutral, acceleration-driven texture, and a louder output stage with a soft peak ceiling. The thumbstick runs from 3V3; follow the full wiring guide for amplifier power and audio connections. The guide also records measured calibration and the installed sound settings. [Handheld layout](docs/handheld.md) describes the broader enclosure/control concept; [verification notes](docs/verification.md) distinguish software checks, observed board behavior and listening feedback.

## Use the bench (Tune)

- Pick **Big-block V8, Straight six, Track four, V-twin, Heavy diesel, Jet turbine, Ion drive**, or **Deep-space hauler**.
- **Start engine** enables live audio. Target RPM and engine load are independent. **Hold to rev** raises both until released; **Rev once** performs a four-second rev. Space or Enter also works while the rev button is focused.
- Shape the exhaust, body, rasp, air, gear whine, spool, roughness, sub rumble, reactor field and metallic resonance. Any layer can be mixed into any powerplant. Cylinder and cycle controls govern the combustion layer; other layers follow shaft speed.
- **Monitor** and **Mute** affect live listening. Exported WAVs retain the synthesizer's full level.
- **Save patch** downloads a versioned JSON file; **Load patch** restores it. Your last settings also persist in this browser at the same local address. The seed reproduces offline renders.
- Choose **Steady RPM**, **Rev up & release**, **Idle to redline**, or **Through the gears**, and a duration of **2–30 seconds**. **Render WAV** downloads **48 kHz, 24-bit, mono PCM** and creates an audition player. Playing the rendered result stops the live engine.

These are synthesized sound-design starting points, not recordings of specific vehicles or a thermodynamic simulation. Cross-bank and uneven firing are expressive approximations. Clips have short edge fades; steady clips are not guaranteed seamless loops. The WAV export's gear trajectory is a scripted sweep, separate from Play's interactive simplified driving model.

The V8 uses two separately resonating exhaust banks, with a close-tailpipe mix that preserves their uneven beat in mono. Load changes pulse shape and exhaust brightness as well as level. **Body**, **Exhaust tone** and **Rasp & grit** control the weight, pipe tuning and loaded bark; acceleration texture fades again at steady revs. [Listen to the 15-second V8 demo](samples/v8-demo.wav): idle, blip, pull, shift, held revs and lift-off. It is rendered at the toy's 16 kHz rate and level-adjusted for comparison, not recorded from the physical speaker.

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
- `tools/render_v8_comparison.mjs`: render the repeatable V8 audition at 16/48 kHz with matched RMS levels. Use `before path/to/old-engine.mjs` for a saved baseline module, then `after` for the current DSP; the latter also updates the checked-in demo.
- `docs/verification.md`: measured results and limitations.

```powershell
npm test
python tools/check_firmware.py
python tools/check_browser.py
python tools/build_standalone.py
python tools/check_standalone.py
python tools/check_audio_startup.py
node tools/render_samples.mjs
```

No audio samples, remote fonts, network services, account, runtime npm install, or Python package installation are required to use Ignition.
