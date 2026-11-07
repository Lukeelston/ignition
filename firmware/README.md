# Ignition handheld — ESP32-S3 bench firmware

Target: the middle board in Luke's 2026-11-06 photo, marked **ESP32-S3-WROOM-1**, Lonely Binary Gold Edition, RGB@IO48. The left board has the same visible layout but its module markings are obscured. The smaller board on the right is a classic ESP32; this firmware's pin map is not for that board.

The [manufacturer's matching board guide](https://learn.lonelybinary.com/boards/esp32-s3) describes an N16R8 variant (16 MB flash, 8 MB PSRAM). A live `esptool flash-id` probe on 2026-11-06 confirmed the connected toy as **ESP32-S3 revision 0.2, 16 MB flash, 8 MB embedded PSRAM**, MAC/USB serial **58:e6:c5:68:65:c8**, then COM9. Re-enumerate ports by serial before each device operation; COM numbers may change. COM7's different serial belongs to the printer display and must not be used. This build retains its conservative 4 MB flash layout and leaves PSRAM disabled. GPIO35–37 and boot-strapping pins remain unused; USB19/20 and UART43/44 are reserved for their USB sockets. Pin assignments were checked against the [manufacturer pinout](https://learn.lonelybinary.com/pinouts/esp32-s3) and [Espressif's board reference](https://documentation.espressif.com/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html).

## Reuse Iris's audio assembly

Luke authorized borrowing the amp and speaker. Read the chat **Design Iris’s embodied form**, then verified its references against the current files:

- `C:/Users/Luke/Atlas/brain/iris/docs/IRIS-SCREEN-HARDWARE.md`
- `C:/Users/Luke/Atlas/brain/iris/satellite/screen-s3/src/speaker.cpp`
- `C:/Users/Luke/Projects/iris-body/DESIGN.md`

These identify the purple **HW-792 / MAX98357A** amplifier and a **39 × 39 × 22 mm** donor speaker. Luke measured approximately 4 Ω; wattage is unknown. The recorded successful Iris setup uses GPIO15/16/17, 32-bit stereo I²S with mono in both slots, and GAIN/SD left open. This firmware keeps those connections and slot format; the current bench rate is 16 kHz, following comparison with its original 32 kHz output. Engine volume is separately adjustable in `board_config.h`; Iris's speech gain is not copied onto continuous engine noise.

Latest bench update (2026-11-07): Luke confirmed the startup, gearing and acceleration-driven texture are working pretty well. The next refinement swaps the stick functions without rewiring: VRx now handles throttle/brake, VRy handles gears, and holding click cycles engine types. Pull is increased 35%; this is toy-model tuning, not measured horsepower. Combustion crank flutter fades out above idle to remove the periodic pitch wobble. The retained boot gain is **0.8**, with a new **2× output makeup gain (+6 dB)** and a soft peak ceiling. Final on-board sound and new controls require the listening/physical test recorded in `docs/verification.md`. Power down before speaker swaps, and keep the speaker across SPK+/SPK−.

Use a spare S3 as the toy controller. Unplug power before moving the amp's five input/power wires, label them, and disconnect the amp from Iris's ESP before connecting it to the toy. Keep the speaker attached to the amp. The amp must not be driven by both ESPs at once. Iris's screen, microphone, board firmware and software stay with Iris. This project has not changed any Iris files or devices.

| MAX98357A connection | Ignition S3 connection |
| --- | --- |
| VIN | USB-powered **5V** |
| GND | GND |
| BCLK | GPIO **15** |
| LRC / LRCLK / WS | GPIO **16** |
| DIN | GPIO **17** |
| GAIN, SD | Leave open, preserving Iris's breakout defaults |
| Speaker + / − | Keep across the amp's two speaker terminals |

Neither speaker terminal goes to GND; this is a bridge output. The amp accepts digital I²S, not an analog audio wire. These details are supported by the [MAX98357 amplifier pin guide](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts) and the existing Iris wiring record. Luke subsequently fitted a TV speaker marked 5 W / 4 Ω; the current bench gain is 0.8, following the clear, loud reference-tone test.

## Handheld controls

Numbers below are **GPIO labels**, not physical header positions. Each switch connects its GPIO to **GND** when active; firmware enables pull-ups and debounces for 20 ms. Open means inactive.

**Current bench wiring:** Luke found that the terminal adapter is labelled for a different board. Use the S3's own pins and bypass that adapter. The build uses only the thumbstick: VRx throttle/brake, VRy flick gears, short click ignition and held click character selection. **No wires move for this update.** GPIO1 is ADC1_CH0 on this S3 ([Espressif datasheet](https://documentation.espressif.com/esp32_s3_datasheet_en.pdf)); optional GPIO5/6 gear buttons remain supported.

```text
ESP32-S3                    THUMBSTICK
3V3 ----------------------- VCC / +5V
GND ----------------------- GND
GPIO4 --------------------- VRy (old throttle direction = gear up; opposite = down)
GPIO1 --------------------- VRx (right = throttle; left = brake)
GPIO8 --------------------- SW (short click = start/stop; hold = cycle engines)

ESP32-S3                    MAX98357A AMP
5V ------------------------ VIN
GND ----------------------- GND
GPIO15 -------------------- BCLK
GPIO16 -------------------- LRC / LRCLK
GPIO17 -------------------- DIN
                            SPK+ ---- speaker ---- SPK-
                            GAIN / SD: leave open
```

Wire with USB unplugged. Amp and thumbstick share the S3 ground. The thumbstick gets **3V3**, even if its supply pin says +5V. The speaker connects only across the amp's two speaker outputs. GPIO5/6/7/9/10 are left empty for this first test. Centre and release the stick, click once to start, move the stick to rev, then click again to stop.

| Control | S3 GPIO | Part / behavior |
| --- | --- | --- |
| Throttle / brake | **1** | Thumbstick VRx; right/high = throttle, left/low = proportional brake; centre = coast/idle |
| Flick gears | **4** | Thumbstick VRy; old throttle/high direction = up, opposite/low = down; re-centre between shifts |
| Gear + | **5** | Normally-open momentary button; one upshift per press |
| Gear − | **6** | Normally-open momentary button; one downshift per press |
| Brake | **7** | Optional separate normally-open button; leave empty for this build |
| Ignition / engine type | **8** | Thumbstick SW; short click/release toggles ignition; hold cycles engines |
| Boost | **9** | Latching toggle; closed = boost |
| Character | **10** | Optional momentary button; cycles all eight sound presets |
| Status RGB | **48** | Already on the board |

### First build: thumbstick and ignition

Luke has a thumbstick, buttons, switches and rotary encoders. The thumbstick replaces the proposed trigger for this bench build. The usual five-pin passive module is supported; confirm the printed labels before connecting. This mapping does not cover an unknown powered joystick controller.

1. Unplug USB/power before adding wires. Use the selected **S3**, with the amp already connected as above.
2. Connect the thumbstick by **labels**, not by its pin order:

| Thumbstick label | S3 connection |
| --- | --- |
| GND | GND (same ground as the amp) |
| VCC / +5V | **3V3**, even if the module label says +5V |
| VRy | **GPIO4**, flick along the old throttle axis for gears |
| SW | **GPIO8**, short click/release starts/stops; hold cycles engines |
| VRx | **GPIO1**, right accelerates and left brakes |

The conventional joystick is two potentiometers plus a switch. Supply it from 3.3 V so its analog outputs stay within the S3 supply rails. The [Joy-IT KY-023 specification](https://www.joy-it.net/en/products/COM-KY023JM) supports 3.3 V operation. The amp still uses 5 V; its supply must not go to the thumbstick or a GPIO. The ADC input has a pull-down.

3. Remove the separate ignition button and move the thumbstick's **SW wire from GPIO7 to GPIO8**. The thumbstick supplies the momentary button internally. Leave GPIO7 unconnected. No external pull-up resistor is needed.
4. Leave the button released. Connect either USB-C socket to the PC; the bench session uses the socket marked **USB**, which identifies itself with the S3 MAC above. Firmware mirrors diagnostics to native **USB** and **UART**. Identify the spare S3 and back up existing flash before its first upload; connecting wires alone does not install it.
5. Open its serial port at **115200 baud**. With the stick released, `throttle=` and `brake=` must settle to **0.00**. Short click/release: the starter spins for about 0.75 seconds, catches with a short RPM flare, and settles to green/running by 1.35 seconds (750 RPM with the default V8). Move right to rev, or flick in the old throttle direction and re-centre to select first before accelerating. Move left to brake; greater travel means more braking. Another short click/release stops, including during startup.

The gear axis must stay within 350 ADC counts of centre (measured near 1915) for 60 ms before arming. A deflection of at least 1000 counts held for 20 ms shifts once; holding it cannot repeat, and reversing directly cannot shift again without re-centring. A held/disconnected stick at boot cannot shift. `gearAxisInverted` can reverse the gear direction. Telemetry reports physical `x_adc`/`y_adc`, `throttle`, proportional `brake`, `gear`, cumulative successful `shifts`, `speed`, `load` and `starter`. Startup, a held click and diagnostic tones inhibit flick shifts.

Neutral free-revs without road speed. In gears 1–6, ratios couple RPM to road speed; each upshift briefly cuts torque, drops RPM and increases engine load at the same throttle. Road speed carries through the shift, with less acceleration in taller gears. Release the throttle to coast. Downshifts that would exceed the rev limit are rejected; first shifts down to neutral. This is a toy vehicle model, not calibrated vehicle physics.

`ignitionIsButton=true` selects this breadboard profile. A short click toggles ignition **on release**, so holding it does not first stop/start the engine. Hold for **650 ms** to select the next engine, then keep holding to advance every **900 ms**; release to keep the chosen type. This works with ignition off or running; release after a hold never toggles power. A boot-held button is ignored until released. A start tap with throttle held at any time is rejected; centring before release cannot turn it into a delayed start. Contact bounce does not toggle. The optional GPIO10 character button still advances once per press. For a maintained toggle, set `ignitionIsButton=false`; that mode keeps its off/on interlock and uses GPIO10 for character selection. Telemetry includes `button_mode`, raw `key`, and `state=0/1/2` for off/cranking/running.

Calibration in `board_config.h` uses **raw ADC counts**: X rest 1905, full throttle 3900, full brake 100, dead zone 300; Y gear centre 1915. These centres come from the released-stick telemetry; observed full travel reached 0/4095. Each side of X normalizes separately from centre to its endpoint, then smooths at 100 Hz. There is no boot auto-calibration that could mistake a held stick for centre. Keep the throttle/brake endpoints on opposite sides of rest. `adc` aliases X for older readers; `x_adc` and `y_adc` retain physical-axis meaning. Rebuild and upload after calibration changes. The [ESP32 ADC reference](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html) explains raw readings and attenuation; raw counts are not calibrated voltages.

A full-travel trigger/pot can still use this firmware by setting rest/full to its measured endpoints and a smaller dead zone. An encoder is not wired or decoded in this profile.

### Add gears and other controls after the first sound test

Add normally-open buttons from **GPIO5 → GND** for gear + and **GPIO6 → GND** for gear −. An optional separate brake button goes **GPIO7 → GND**. On a four-leg tactile button, use terminals that are disconnected until pressed; the two legs on a connected pair would permanently hold the input. Add a boost toggle **GPIO9 → GND** and an optional character button **GPIO10 → GND**. All grounds may share a ground rail.

Boot is silent. Centre the stick and **tap ignition**. Start in neutral, tap gear +, then push the stick. The automatic clutch handles launch. Gear − back from first enters neutral; no reverse gear. A released stick idles the engine in neutral; a moving toy drivetrain still coasts until braked.

RGB: dim blue = off, amber = cranking, green = running, purple = boost, red = rejected downshift/rev limit or audio fault (serial distinguishes them). Gear/RPM and the selected preset print over UART. This first firmware uses the existing LED, not a fabricated screen driver. A dedicated gear display is a later hardware addition.

## Build and tune

Compiled with installed **Arduino CLI 1.5.1**, **Espressif Arduino core 3.3.12**, ESP32-S3 at 240 MHz, 4 MB partitions, PSRAM disabled, and hardware USB CDC enabled on boot. USB logging is best-effort with a zero write timeout so a stopped monitor cannot stall the controls. No third-party Arduino libraries. Run from the project root:

```powershell
python tools/build_firmware.py
```

This **only compiles**. It neither picks a COM port nor uploads anything. Output is in `firmware/build/esp32s3`; build log in `firmware/build/compile.log`. Identify the actual connected spare S3 and preserve any existing firmware before the first upload. The live toy identity is recorded above; see `docs/verification.md` for the latest upload and bench evidence.

For a bounded, read-only capture over the native USB socket (requires Python `pyserial`):

```powershell
python tools/read_board.py --serial 58:e6:c5:68:65:c8 --seconds 8
```

This helper opens only the matching USB identity, does not deliberately reset or write to the board, saves parsed telemetry in `evidence/board-*.json`, and closes the port afterward. It exits unsuccessfully if fewer than two valid samples arrive. Parsing is separate from audio verification: advancing blocks do not prove sound at the speaker.

Add `--live` to print each arriving sample during a guided bench test. Telemetry is 1 Hz; hold an input for at least two seconds when checking its raw level. Brief button clicks can fall between samples even though the firmware polls and debounces at 100 Hz.

For a live engine-level comparison, add `--gain 0.8 --live`. This **writes** an engine gain command without restarting the board or starting the engine. Accepted values are 0.0 through 0.8 in steps of 0.1; native USB digits `0` through `8` do the same. Existing smoothing ramps the change. Commands are RAM-only and reset to the configured **0.8** at reboot. Keep the speaker away from your ear when raising it. Diagnostic beep level is independent of this setting.

`pcm_peak` and `pcm_rms` measure the actual packed samples sent to I²S, normalized to full scale. Each report samples one 128-frame (8 ms at 16 kHz) block; it is **not** a peak/RMS measurement over the entire second and can miss intervening peaks. These fields establish digital signal level, not speaker volume or amplifier headroom.

For an output-path comparison, add `--tone --seconds 15`. This **writes** a `t` command to the identified toy: the engine stops and eight quiet 660 Hz beeps play for about 12 seconds, with soft edges and silence between them. The test uses the configured sample rate and 32-bit stereo I²S path but bypasses the engine sound source. It finishes with the engine off; release and click the stick to start normally again. No diagnostic sound plays automatically at boot. Telemetry includes numeric `tone` (active) and `tone_request` fields. Clean beeps would narrow the investigation toward the engine sound or frequency-dependent speaker response; distorted beeps would point toward the output path. Neither result alone identifies a particular faulty component.

**Current bench state:** sample rate remains **16 kHz**, the rate used in the user-accepted toy profile. Earlier distorted-beep trials were superseded by the clear/loud Iris reference and PCM packing/source-texture corrections. The fixed sample-count engine delay and gain smoothing still reflect this rate; no clock or I2S changes are part of this refinement.

The engine retains the clean reference's signed PCM16 conversion in the high 16 bits of each 32-bit slot and six 320-frame DMA buffers. Luke heard the same static in the original host render and confirmed that disabling random intake/firing texture removed it. Combustion presets now envelope those random components using positive RPM acceleration, with about 60 ms attack and 350 ms release, while preserving the deterministic engine tone and the intake/roughness depth controls. Steady revs no longer sustain the random hiss. Turbine/ion texture remains unchanged. The starter motor is a separate deterministic firmware layer; browser and firmware share catch timing/RPM behavior, but this dedicated motor layer is currently firmware-only.

Combustion crank flutter now fades from full at idle to zero at twice idle RPM. Previously the default V8's 4 Hz flutter perturbed a commanded 4000 RPM to approximately 3970–4030 firing RPM throughout the climb. The new firmware output stage doubles engine/starter amplitude below its 0.8 knee, then smoothly limits peaks below 0.96 full scale. It has no time-varying compressor gain that could introduce pumping. Diagnostic tones bypass this boost. `volume=0.8` and `makeup=2.0` identify the current profile; lowering live gain still works. Acoustic loudness and amplifier/speaker behavior require listening, not just digital bounds.

The browser's Tune controls remain the patch editor. Save a patch, then append it as the ninth firmware character:

```powershell
node tools/export_firmware_patch.mjs C:/path/to/ignition-custom-patch.json
python tools/build_firmware.py
```

Running the exporter with no argument restores the original eight presets. All sound controls and seed are carried into the generated header; live RPM/load come from the thumbstick and drivetrain. Firmware edits use C++ and the Arduino toolchain, not JavaScript on the board.

## Verification and limits

The portable core has no device dependencies and runs under host g++ for checks:

```powershell
python tools/check_firmware.py
python tools/check_stick_shift.py
python tools/check_starter_sound.py
node --test tests/*.test.mjs
```

Checks cover centre dead zones/endpoints, neutral/drive/shift/proportional brake/boost, stronger launch, rejected over-rev, motion-preserving character changes, short-click release, long-hold/repeat without power toggles, cancellation, boot-held interlock, throttle-invalid presses, contact bounce, millis wrap, maintained-switch compatibility, and one shift per gear excursion. DSP checks cover all eight patches, louder-output headroom, seed determinism, firing frequency, idle-only flutter, acceleration texture and silence. A 20-second drive trace is compared at 20 checkpoints against the browser model. The generated WAV is **host synthesis**, not a recording from the ESP.

Audio runs on a separate task with 128 frames per synthesis block, six 320-frame DMA buffers, duplicated 32-bit stereo slots and continuous silent clocks while off. DMA capacity is currently 120 ms at 16 kHz, compared with the original four 128-frame buffers' 32 ms; the increased capacity can also increase control-to-sound latency. The original 32 kHz rate gives a 4,000 µs synthesis-block deadline; the current 16 kHz bench comparison gives 8,000 µs. No Wi-Fi or Bluetooth is initialized. Stale input updates ramp to silence after 250 ms; an I²S write fault latches mute until reboot. Serial reports maximum render time, blocks exceeding the configured synthesis deadline, and write errors. Those counters do not independently prove that no DMA underrun or acoustic glitch occurred.

Board identity, a verified full-flash backup, flash readback and physical start/rev/up/downshift response passed for the previous profile, which Luke described as working pretty well. See `docs/verification.md` for the latest evidence. **The louder output, remapped controls and held-click cycling need a fresh physical/listening check.** Verify both pedals, gears, character cycling/release, short-click stop and smooth rev climb; inspect the 8000 µs render budget and error counters alongside actual sound. No battery or enclosure has been built yet.
