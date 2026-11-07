#pragma once
// Lonely Binary ESP32-S3 Gold Edition / ESP32-S3-WROOM-1.
// GPIO numbers, not connector position numbers. Not the classic ESP32 pinout.
namespace board {
constexpr int throttle=4,gearUp=5,gearDown=6,brake=7,ignitionSwitch=8,boost=9,character=10;
constexpr bool ignitionIsButton=true; // GPIO8 -> momentary button -> GND; tap to start/stop
// Preserve Iris's MAX98357A harness and duplicate mono into both 32-bit slots.
constexpr int bclk=15,wordSelect=16,dataOut=17,rgb=48;
// Thumbstick-only bench: VRy -> GPIO4, SW -> ignition GPIO8, supply -> 3V3.
// Leave optional brake GPIO7 and other controls unconnected until needed.
// Provisional raw ADC calibration. Record rest and desired full-forward readings.
// A full reading below rest reverses direction without changing centre = idle.
constexpr int throttleRest=2048,throttleFull=3900,throttleDeadZone=300;
constexpr float volume=.08f; // reduced-level bench setting while investigating crackle
constexpr int blockFrames=128;
}
