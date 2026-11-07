#include "../firmware/ignition/starter_sound.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

using ignition::StarterSound;
static int failures = 0;
static int checks = 0;

static void expect(bool condition, const char* message) {
  ++checks;
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}

static void toneAndBounds() {
  StarterSound sound(16000);
  float peak = 0, previous = 0, beforePrevious = 0;
  double energy = 0, curvatureEnergy = 0;
  bool valid = true;
  int earlyCrossings = 0, lateCrossings = 0;
  for (int i = 0; i < 32000; ++i) {
    const float sample = sound.next(1);
    valid = valid && std::isfinite(sample) && std::fabs(sample) <= .14f;
    peak = std::max(peak, std::fabs(sample));
    if (i >= 1600) {
      energy += sample * sample;
      const float curvature = sample - 2 * previous + beforePrevious;
      curvatureEnergy += curvature * curvature;
    }
    if (previous <= 0 && sample > 0) {
      if (i >= 480 && i < 2080) ++earlyCrossings;
      if (i >= 6400 && i < 8000) ++lateCrossings;
    }
    beforePrevious = previous;
    previous = sample;
  }
  expect(valid, "all motor samples are finite and bounded by .14");
  expect(peak > .04f && energy / 30400 > .0002,
         "starter produces an audible, nontrivial motor signal");
  expect(earlyCrossings >= 20 && earlyCrossings <= 45 &&
         lateCrossings >= 20 && lateCrossings <= 45,
         "motor fundamental stays in the 200-450 Hz range");
  expect(lateCrossings > earlyCrossings + 3,
         "motor pitch rises during initial spin-up");
  expect(energy > 0 && curvatureEnergy / energy < .02,
         "motor is tonal without a broadband hiss bed or hard waveform steps");
}

static void edgesAtRate(uint32_t rate) {
  StarterSound sound(rate);
  bool idleSilent = true;
  for (uint32_t i = 0; i < rate / 4; ++i) idleSilent &= sound.next(0) == 0;
  expect(idleSilent, "inactive starter is exactly silent");
  expect(std::fabs(sound.next(1)) <= .14f / (rate * .005f),
         "starting uses a gradual amplitude edge");
  for (uint32_t i = 1; i < rate / 3; ++i) sound.next(1);
  StarterSound reference = sound;
  int lastNonzero = -1;
  bool ratioValid = true;
  bool hadSignalAfterCancel = false;
  for (uint32_t i = 0; i < rate / 25; ++i) {
    const float held = reference.next(1);
    const float faded = sound.next(0);
    if (faded != 0) lastNonzero = int(i);
    if (i < rate / 250 && std::fabs(faded) > .001f) hadSignalAfterCancel = true;
    // Compare the same oscillator phase with its unfaded copy. An abrupt cut,
    // asymptotic envelope or rate-dependent ramp violates these broad limits.
    if (std::fabs(held) > .001f) {
      const float fraction = faded / held;
      if (i >= rate * .002f && i < rate * .003f)
        ratioValid &= fraction > .35f && fraction < .9f;
      if (i >= rate * .012f) ratioValid &= faded == 0;
    }
  }
  expect(hadSignalAfterCancel && ratioValid,
         "cancelling smooths the existing signal then becomes exactly zero");
  expect(lastNonzero >= int(rate * .005f) && lastNonzero <= int(rate * .015f),
         "cancel fade lasts 5-15 milliseconds at each sample rate");
  bool staysSilent = true;
  for (uint32_t i = 0; i < rate; ++i) staysSilent &= sound.next(0) == 0;
  expect(staysSilent, "cancelled sound stays at exact zero without a residual tail");
}

static void deterministicRestart() {
  StarterSound sound(16000), neverStarted(16000);
  std::vector<float> first;
  for (int i = 0; i < 6400; ++i) first.push_back(sound.next(1));
  sound.reset();
  bool resetMatches = true;
  for (float sample : first) resetMatches &= sample == sound.next(1);
  expect(resetMatches, "explicit reset exactly repeats the first starting sound");

  for (int i = 0; i < 32000; ++i) sound.next(0);
  bool naturalRestartMatches = true;
  for (float sample : first) naturalRestartMatches &= sample == sound.next(1);
  expect(naturalRestartMatches, "finished cancel fade restores repeatable restart phase");

  sound.reset();
  for (int i = 0; i < 16000; ++i) sound.next(0);
  bool idlePreservesPhase = true;
  for (int i = 0; i < 1600; ++i)
    idlePreservesPhase &= sound.next(1) == neverStarted.next(1);
  expect(idlePreservesPhase, "time spent inactive cannot advance the motor phase");
}

static void controlsAndInterruptions() {
  StarterSound full(16000), excessive(16000), quiet(16000);
  double fullEnergy = 0, quietEnergy = 0;
  bool clamped = true;
  for (int i = 0; i < 16000; ++i) {
    const float sample = full.next(1);
    clamped &= sample == excessive.next(3);
    const float lower = quiet.next(.25f);
    if (i > 320) { fullEnergy += sample * sample; quietEnergy += lower * lower; }
  }
  expect(clamped, "requested level above one is clamped without boosting peak");
  expect(fullEnergy > 0 && std::fabs(quietEnergy / fullEnergy - .0625) < .001,
         "fractional requested level controls amplitude");
  for (float invalid : {-1.f, std::numeric_limits<float>::infinity(),
                        -std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
    StarterSound cancelled = full, invalidLevel = full;
    bool matches = true;
    for (int i = 0; i < 1000; ++i)
      matches &= cancelled.next(0) == invalidLevel.next(invalid);
    expect(matches, "invalid or negative level fades to silence without poisoning state");
  }
  bool interruptionsSafe = true;
  for (int cancelFrame : {1, 15, 79, 160, 3200, 9599, 12799}) {
    StarterSound interrupted(16000);
    for (int i = 0; i < cancelFrame; ++i) interrupted.next(1);
    for (int i = 0; i < 400; ++i) {
      const float sample = interrupted.next(0);
      interruptionsSafe &= std::isfinite(sample) && std::fabs(sample) <= .14f;
      if (i >= 240) interruptionsSafe &= sample == 0;
    }
  }
  expect(interruptionsSafe, "cancel works during attack, steady cranking and handoff");

  StarterSound zeroRate(0), normalRate(16000);
  bool defaultRate = true;
  for (int i = 0; i < 1600; ++i) defaultRate &= zeroRate.next(1) == normalRate.next(1);
  expect(defaultRate, "invalid zero sample rate uses the normal 16 kHz rate");
}

int main() {
  toneAndBounds();
  for (uint32_t rate : {8000u, 16000u, 48000u}) edgesAtRate(rate);
  deterministicRestart();
  controlsAndInterruptions();
  if (failures) {
    std::fprintf(stderr, "%d of %d starter sound checks failed\n", failures, checks);
    return 1;
  }
  std::printf("PASS: %d starter sound checks; portable audio only, no hardware test\n", checks);
}
