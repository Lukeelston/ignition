#pragma once
// Lonely Binary ESP32-S3 Gold Edition / ESP32-S3-WROOM-1.
// GPIO numbers, not connector position numbers. Not the classic ESP32 pinout.
namespace board {
constexpr int throttle=1,gearUp=5,gearDown=6,brake=7,ignitionSwitch=8,boost=9,character=10;
constexpr int gearAxis=4; // Same wiring: VRx pedals, VRy flick gears.
constexpr int gearCenter=1915,gearTrigger=1000,gearDeadZone=350;
constexpr bool gearAxisInverted=false; // Old throttle direction now shifts up.
constexpr bool ignitionIsButton=true; // short release=start/stop; hold=cycle character
// Preserve Iris's MAX98357A harness and duplicate mono into both 32-bit slots.
constexpr int bclk=15,wordSelect=16,dataOut=17,rgb=48;
// Thumbstick-only bench: VRx -> GPIO1, VRy -> GPIO4, SW -> GPIO8, supply -> 3V3.
// Leave optional brake GPIO7 and other controls unconnected until needed.
// Released X/Y were measured near 1905/1915; full travel reached 0 and 4095.
constexpr int throttleRest=1905,throttleFull=3900,brakeFull=100,throttleDeadZone=300;
constexpr float volume=.8f; // retain the listening-test gain across power cycles
constexpr float outputMakeup=2.f; // +6 dB before the speaker's soft peak ceiling
constexpr int blockFrames=128;
}
