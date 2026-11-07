# Ignition handheld — ESP32-S3 bench firmware

Target: the middle board in Luke's 2026-11-06 photo, marked **ESP32-S3-WROOM-1**, Lonely Binary Gold Edition, RGB@IO48. The left board has the same visible layout but its module markings are obscured. The smaller board on the right is a classic ESP32; this firmware's pin map is not for that board.

The [manufacturer's matching board guide](https://learn.lonelybinary.com/boards/esp32-s3) describes an N16R8 variant (16 MB flash, 8 MB PSRAM). A live `esptool flash-id` probe on 2026-11-06 confirmed the connected toy as **ESP32-S3 revision 0.2, 16 MB flash, 8 MB embedded PSRAM**, MAC/USB serial **58:e6:c5:68:65:c8**, then COM9. Re-enumerate ports by serial before each device operation; COM numbers may change. COM7's different serial belongs to the printer display and must not be used. This build retains its conservative 4 MB flash layout and leaves PSRAM disabled. GPIO35–37 and boot-strapping pins remain unused; USB19/20 and UART43/44 are reserved for their USB sockets. Pin assignments were checked against the [manufacturer pinout](https://learn.lonelybinary.com/pinouts/esp32-s3) and [Espressif's board reference](https://documentation.espressif.com/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html).

## Reuse Iris's audio assembly

Luke authorized borrowing the amp and speaker. Read the chat **Design Iris’s embodied form**, then verified its references against the current files:

- `C:/Users/Luke/Atlas/brain/iris/docs/IRIS-SCREEN-HARDWARE.md`
- `C:/Users/Luke/Atlas/brain/iris/satellite/screen-s3/src/speaker.cpp`
- `C:/Users/Luke/Projects/iris-body/DESIGN.md`

These identify the purple **HW-792 / MAX98357A** amplifier and a **39 × 39 × 22 mm** donor speaker. Luke measured approximately 4 Ω; wattage is unknown. The recorded successful Iris setup uses GPIO15/16/17, 32-bit stereo I²S with mono in both slots, and GAIN/SD left open. This firmware keeps those connections and slot format; its sample rate is 32 kHz, versus Iris's 16 kHz speech. Its conservative starting volume is separately adjustable in `board_config.h`; Iris's speech gain is not copied onto continuous engine noise.

Latest bench update (2026-11-06): after bypassing the mismatched terminal adapter and using the thumbstick click on GPIO8, Luke confirmed audible engine sound. He substituted a tiny unmarked rectangular speaker and reported buzz/rattle. That replacement does **not** inherit the original donor's measured impedance. The firmware gain was reduced from 0.22 to **0.08** for a comparison test; the reduced value is flashed and confirmed in live telemetry. Luke then found a TV speaker marked **5 W, 4 Ω**. This is a compatible load for the MAX98357A; its 5 W rating is power handling, not required drive power. On 5 V the amp is specified around 2.5 W into 4 Ω at 1% THD. Power down before swapping, and keep the speaker across SPK+/SPK−. The TV speaker's listening result is still pending.

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

Neither speaker terminal goes to GND; this is a bridge output. The amp accepts digital I²S, not an analog audio wire. These details are supported by the [MAX98357 amplifier pin guide](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts) and the existing Iris wiring record. The speaker's unknown wattage means the first listening test should use the configured low level and a short run.

## Handheld controls

Numbers below are **GPIO labels**, not physical header positions. Each switch connects its GPIO to **GND** when active; firmware enables pull-ups and debounces for 20 ms. Open means inactive.

**Current bench wiring:** Luke found that the terminal adapter is labelled for a different board. Use the S3's own pins and bypass that adapter. The current build uses only the thumbstick and its click for ignition; the separate button/switch has been removed from the wiring plan. The already-installed button-mode firmware supports this without a reflash.

```text
ESP32-S3                    THUMBSTICK
3V3 ----------------------- VCC / +5V
GND ----------------------- GND
GPIO4 --------------------- VRy
GPIO8 --------------------- SW (click = start/stop)
                            VRx: not connected

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
| Throttle | **4** | Thumbstick VRy; centre = idle, one direction = throttle |
| Gear + | **5** | Normally-open momentary button; one upshift per press |
| Gear − | **6** | Normally-open momentary button; one downshift per press |
| Brake | **7** | Optional separate normally-open button; leave empty for this build |
| Ignition | **8** | Thumbstick SW click; tap to start, tap again to stop |
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
| VRy | **GPIO4** |
| SW | **GPIO8**, click once to start, again to stop |
| VRx | Leave disconnected |

The conventional joystick is two potentiometers plus a switch. Supply it from 3.3 V so its analog outputs stay within the S3 supply rails. The [Joy-IT KY-023 specification](https://www.joy-it.net/en/products/COM-KY023JM) supports 3.3 V operation. The amp still uses 5 V; its supply must not go to the thumbstick or a GPIO. The ADC input has a pull-down.

3. Remove the separate ignition button and move the thumbstick's **SW wire from GPIO7 to GPIO8**. The thumbstick supplies the momentary button internally. Leave GPIO7 unconnected. No external pull-up resistor is needed.
4. Leave the button released. Connect either USB-C socket to the PC; the bench session uses the socket marked **USB**, which identifies itself with the S3 MAC above. Firmware mirrors diagnostics to native **USB** and **UART**. Identify the spare S3 and back up existing flash before its first upload; connecting wires alone does not install it.
5. Open its serial port at **115200 baud**. With the stick released, `throttle=` must settle to **0.00**. Click and release the thumbstick: expect amber cranking followed by green and approximately 750 RPM with the default V8. The engine stays on after release. Push the stick to rev in neutral; release to return to idle. Click again to stop, including while revving or cranking. The already-installed button-mode firmware needs no upload for this wiring change.

`ignitionIsButton=true` in `board_config.h` selects this breadboard profile. A button held during boot cannot start the engine until released and pressed again. A press with throttle held is rejected; centring the stick while still holding the button does not start it later. Holding or ordinary contact bounce does not repeat the toggle. For a future maintained toggle, set `ignitionIsButton=false`, rebuild and upload; that mode requires off/on and stays on only while the switch is closed. Telemetry includes `button_mode=1` for the button profile, raw `key=0/1`, and `state=0/1/2` for off/cranking/running.

Default calibration in `board_config.h`: `throttleRest=2048`, `throttleFull=3900`, `throttleDeadZone=300`, all **raw ADC counts**. These are provisional, not a measurement of Luke's stick. Record `adc=` at centre and at the desired full-forward position; put those readings in rest/full. A full reading below rest automatically reverses travel. The centre dead zone and the entire opposite direction produce zero throttle. There is no boot auto-calibration that could mistake a held stick for centre. If motion is sideways, use VRx instead of VRy on GPIO4; if backwards, change the full endpoint. Rebuild and upload after calibration changes. The [ESP32 ADC reference](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html) explains raw 12-bit readings and attenuation; raw counts are not calibrated voltages.

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
node --test tests/*.test.mjs
```

Checks cover centred-stick idle/dead zone/opposite travel, either endpoint direction, invalid calibration, stick-driven ignition/rev/return to idle, neutral/drive/shift/brake/boost, rejected over-rev, button tap/release latching, stop at full throttle, cancel during cranking, boot-held button, rejected start with held throttle, no delayed start or hold-repeat, contact bounce, maintained-switch compatibility, one shift per held gear button, finite bounded sound for eight patches, seed determinism, firing frequency and fading to exact silence. A 20-second drive trace is compared at 20 checkpoints against the browser model. The generated WAV is **host synthesis**, not a recording from the ESP.

Audio runs on a separate task with 128 frames per block, four DMA buffers, 32 kHz output, duplicated 32-bit stereo slots and continuous silent clocks while off. No Wi-Fi or Bluetooth is initialized. Stale input updates ramp to silence after 250 ms; an I²S write fault latches mute until reboot. Serial reports maximum render time, blocks exceeding the 4,000 µs synthesis deadline, and write errors. Those counters do not independently prove that no DMA underrun or acoustic glitch occurred.

Board identity, a verified full-flash backup, flash readback, boot and live centred-stick zero throttle have passed on the connected S3. See `docs/verification.md` for measurements. **Physical sound and the full control sequence are unverified — need user confirmation and a bench test.** Remaining acceptance: confirm full stick travel, brake click and one shift per press; capture/listen to V8 and ion sounds; run active synthesis for two minutes with `render_max_us < 4000`, `render_late=0`, `write_errors=0`, no audible glitches; verify ignition-off silence and boot interlock; then assess comfortable volume and speaker heating. No battery or enclosure has been built yet.
