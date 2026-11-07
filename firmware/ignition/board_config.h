#pragma once
// Lonely Binary ESP32-S3 Gold Edition / ESP32-S3-WROOM-1.
// GPIO numbers, not connector position numbers. Not the classic ESP32 pinout.
namespace board {
constexpr int throttle=4,gearUp=5,gearDown=6,brake=7,ignitionSwitch=8,boost=9,character=10;
// Preserve Iris's MAX98357A harness and duplicate mono into both 32-bit slots.
constexpr int bclk=15,wordSelect=16,dataOut=17,rgb=48;
// Spring-centred thumbstick: VRy -> GPIO4, SW -> brake GPIO7, supply -> 3V3.
// Provisional raw ADC calibration. Record rest and desired full-forward readings.
// A full reading below rest reverses direction without changing centre = idle.
constexpr int throttleRest=2048,throttleFull=3900,throttleDeadZone=300;
constexpr float volume=.22f; // conservative initial bench level; tune after listening
constexpr int blockFrames=128;
}
