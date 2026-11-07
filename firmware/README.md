# Ignition handheld — ESP32-S3 bench firmware

Target: the middle board in Luke's 2026-11-06 photo, marked **ESP32-S3-WROOM-1**, Lonely Binary Gold Edition, RGB@IO48. The left board has the same visible layout but its module markings are obscured. The smaller board on the right is a classic ESP32; this firmware's pin map is not for that board.

The [manufacturer's matching board guide](https://learn.lonelybinary.com/boards/esp32-s3) describes an N16R8 variant (16 MB flash, 8 MB PSRAM). The photo does not establish this particular unit's capacity, so this build uses a 4 MB flash layout and no PSRAM. Runtime prints detected flash size. GPIO35–37, boot-strapping pins, native USB19/20 and UART43/44 remain unused. Pin assignments were checked against the [manufacturer pinout](https://learn.lonelybinary.com/pinouts/esp32-s3) and [Espressif's board reference](https://documentation.espressif.com/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html).

## Reuse Iris's audio assembly

Luke authorized borrowing the amp and speaker. Read the chat **Design Iris’s embodied form**, then verified its references against the current files:

- `C:/Users/Luke/Atlas/brain/iris/docs/IRIS-SCREEN-HARDWARE.md`
- `C:/Users/Luke/Atlas/brain/iris/satellite/screen-s3/src/speaker.cpp`
- `C:/Users/Luke/Projects/iris-body/DESIGN.md`

These identify the purple **HW-792 / MAX98357A** amplifier and a **39 × 39 × 22 mm** donor speaker. Luke measured approximately 4 Ω; wattage is unknown. The recorded successful Iris setup uses GPIO15/16/17, 32-bit stereo I²S with mono in both slots, and GAIN/SD left open. This firmware keeps those connections and slot format; its sample rate is 32 kHz, versus Iris's 16 kHz speech. Its conservative starting volume is separately adjustable in `board_config.h`; Iris's speech gain is not copied onto continuous engine noise.

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

| Control | S3 GPIO | Part / behavior |
| --- | --- | --- |
| Throttle | **4** | Analog spring-return trigger, or 10 kΩ linear pot for the bench |
| Gear + | **5** | Normally-open momentary button; one upshift per press |
| Gear − | **6** | Normally-open momentary button; one downshift per press |
| Brake | **7** | Normally-open momentary button; hold to brake |
| Ignition | **8** | Latching toggle; closed = on |
| Boost | **9** | Latching toggle; closed = boost |
| Character | **10** | Optional momentary button; cycles all eight sound presets |
| Status RGB | **48** | Already on the board |

Pot wiring: one outer terminal to **3V3**, the other to **GND**, centre/wiper to **GPIO4**. Use 3.3 V control signals only. For a trigger module, check its supply/output requirements before substituting it for the pot. The input has a pull-down, and the calibration/dead zone is configurable. Never feed the amp's 5 V supply into a GPIO.

To calibrate, watch `adc=` at 115200 baud while fully releasing and squeezing the trigger. Put those raw endpoints in `triggerReleased` and `triggerFull`; set `triggerReversed` if travel runs backward. Defaults 150/3900 are provisional bench endpoints. A pot is sufficient for the first sound test; the spring mechanism and enclosure remain physical design work.

Power the board through its **UART** USB-C socket for the first bench session. Boot is silent. Release throttle, switch ignition **off**, then **on**. A held-on switch at boot cannot start the engine. An on edge with throttle held is rejected until another off/on cycle. Start in neutral, tap gear +, then squeeze. The automatic clutch handles launch. Gear − back from first enters neutral; no reverse gear.

RGB: dim blue = off, amber = cranking, green = running, purple = boost, red = rejected downshift/rev limit or audio fault (serial distinguishes them). Gear/RPM and the selected preset print over UART. This first firmware uses the existing LED, not a fabricated screen driver. A dedicated gear display is a later hardware addition.

## Build and tune

Compiled with installed **Arduino CLI 1.5.1**, **Espressif Arduino core 3.3.12**, ESP32-S3 at 240 MHz, 4 MB partitions and PSRAM disabled. No third-party Arduino libraries. Run from the project root:

```powershell
python tools/build_firmware.py
```

This **only compiles**. It neither picks a COM port nor uploads anything. Output is in `firmware/build/esp32s3`; build log in `firmware/build/compile.log`. Identify the actual connected spare S3 and preserve any existing firmware before the first upload. No device identity has been verified and no upload has occurred in this phase.

The browser's Tune controls remain the patch editor. Save a patch, then append it as the ninth firmware character:

```powershell
node tools/export_firmware_patch.mjs C:/path/to/ignition-custom-patch.json
python tools/build_firmware.py
```

Running the exporter with no argument restores the original eight presets. All sound controls and seed are carried into the generated header; live RPM/load come from the trigger and drivetrain. Firmware edits use C++ and the Arduino toolchain, not JavaScript on the board.

## Verification and limits

The portable core has no device dependencies and runs under host g++ for checks:

```powershell
python tools/check_firmware.py
node --test tests/*.test.mjs
```

Checks cover neutral/drive/shift/brake/boost, rejected over-rev, ignition at boot, held throttle on start, switch bounce, one shift per held button, finite bounded sound for eight patches, seed determinism, firing frequency and fading to exact silence. A 20-second drive trace is compared at 20 checkpoints against the browser model. The generated WAV is **host synthesis**, not a recording from the ESP.

Audio runs on a separate task with 128 frames per block, four DMA buffers, 32 kHz output, duplicated 32-bit stereo slots and continuous silent clocks while off. No Wi-Fi or Bluetooth is initialized. Stale input updates ramp to silence after 250 ms; an I²S write fault latches mute until reboot. Serial reports maximum render time, blocks exceeding the 4,000 µs synthesis deadline, and write errors. Those counters do not independently prove that no DMA underrun or acoustic glitch occurred.

**Hardware is unverified — needs user confirmation and a physical bench test.** Acceptance: identify and back up the target board; confirm trigger endpoints and one shift per press; capture/listen to V8 and ion sounds; run two minutes with `render_max_us < 4000`, `render_late=0`, `write_errors=0`, no audible glitches; verify ignition-off silence and boot interlock; then assess comfortable volume and speaker heating. No battery, enclosure or printed trigger has been built yet.
