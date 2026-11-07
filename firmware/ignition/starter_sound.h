#pragma once
#include <cmath>
#include <cstdint>

namespace ignition {
// Independent of engine RPM: a quiet geared-motor whirr with compression pulses.
// The caller owns cranking/catch timing and passes its requested 0..1 level for
// every audio sample. Cancellation takes about 10 ms, then stays exactly zero.
// No allocations, random noise, lookup tables or per-sample transcendental math.
class StarterSound {
  const float sampleSeconds;
  const float levelStep;
  float level = 0, spin = 0, motorPhase = .25f, compressionPhase = 0;

  // Rounded triangle: continuous value and slope even at its turning points.
  // Both this and its second harmonic are bounded by +/-1.
  static float roundedTriangle(float phase) {
    const float distance = phase > .5f ? phase - .5f : .5f - phase;
    const float triangle = 1 - 4 * distance;
    return triangle * (1.5f - .5f * triangle * triangle);
  }

public:
  explicit StarterSound(uint32_t rate = 16000)
      : sampleSeconds(1.f / (rate >= 1000 ? rate : 16000)),
        levelStep(100 * sampleSeconds) {}

  float next(float requestedLevel) {
    const float target = !std::isfinite(requestedLevel) || requestedLevel < 0 ? 0
                         : requestedLevel > 1 ? 1 : requestedLevel;
    const float change = target - level;
    if (change > levelStep) level += levelStep;
    else if (change < -levelStep) level -= levelStep;
    else level = target;
    if (level == 0) {
      reset();
      return 0;
    }

    // Gentle spin-up; compression slows the motor and modulates its loudness.
    spin += sampleSeconds / .32f;
    if (spin > 1) spin = 1;
    const float compression = .5f + .5f * roundedTriangle(compressionPhase);
    const float frequency = 265 + 125 * spin - 40 * compression;
    float harmonicPhase = motorPhase * 2;
    if (harmonicPhase >= 1) harmonicPhase -= 1;
    const float motor = .096f * roundedTriangle(motorPhase)
                      + .024f * roundedTriangle(harmonicPhase);
    // Absolute peak <= (.096 + .024) * 1 * 1 = .12, leaving mixing headroom.
    const float output = level * (.4f + .6f * compression) * motor;
    motorPhase += frequency * sampleSeconds;
    if (motorPhase >= 1) motorPhase -= 1;
    compressionPhase += (7.5f + 2.5f * spin) * sampleSeconds;
    if (compressionPhase >= 1) compressionPhase -= 1;
    return output;
  }

  // Use while silent. During playback, next(0) provides a click-free stop.
  void reset() {
    level = spin = compressionPhase = 0;
    motorPhase = .25f;
  }
};
}
