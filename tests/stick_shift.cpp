#include "../firmware/ignition/stick_shift.h"
#include <cstdio>
#include <initializer_list>
#include <limits>

using ignition::StickShift;
using ignition::StickShiftConfig;

static int failures = 0;
static int scenarios = 0;

static void expect(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    ++failures;
  }
}

struct Sample { int raw; uint32_t now; int event; bool armed; };

static void trace(const char* name, std::initializer_list<Sample> samples,
                  const StickShiftConfig& config = StickShiftConfig()) {
  ++scenarios;
  StickShift stick(config);
  expect(!stick.armed(), "boot must start disarmed");
  for (const auto& sample : samples) {
    const int event = stick.update(sample.raw, sample.now);
    if (event != sample.event || stick.armed() != sample.armed) {
      std::fprintf(stderr,
                   "FAIL: %s at %u ms, ADC %d: event %d (want %d), armed %d (want %d)\n",
                   name, sample.now, sample.raw, event, sample.event,
                   stick.armed(), sample.armed);
      ++failures;
    }
  }
}

int main() {
  // Removing either dwell would admit startup noise or a brief accidental touch.
  trace("startup center dwell and debounced right flick", {
    {2048, 0, 0, false}, {2048, 59, 0, false}, {2048, 60, 0, true},
    {4095, 70, 0, true}, {4095, 89, 0, true}, {4095, 90, 1, false},
    {4095, 110, 0, false}, {4095, 10000, 0, false}
  });

  trace("held left at boot cannot shift until centered", {
    {0, 0, 0, false}, {0, 20, 0, false}, {0, 5000, 0, false},
    {4095, 6000, 0, false}, {4095, 6100, 0, false},
    {2048, 6200, 0, false}, {2048, 6259, 0, false}, {2048, 6260, 0, true},
    {0, 6270, 0, true}, {0, 6290, -1, false}, {0, 6500, 0, false}
  });

  trace("held right at boot cannot shift until centered", {
    {4095, 0, 0, false}, {4095, 1000, 0, false},
    {2048, 1010, 0, false}, {2048, 1070, 0, true},
    {4095, 1080, 0, true}, {4095, 1100, 1, false}
  });

  trace("disconnected ADC pulldown never arms", {
    {0, 0, 0, false}, {0, 60, 0, false}, {0, 1000, 0, false},
    {0, 1000000, 0, false}, {0, 4000000000u, 0, false}
  });

  trace("outer threshold bounce restarts its debounce", {
    {2048, 0, 0, false}, {2048, 60, 0, true},
    {3048, 70, 0, true}, {3047, 89, 0, true},
    {3048, 90, 0, true}, {3048, 109, 0, true}, {3048, 110, 1, false}
  });

  trace("direction change restarts pending debounce", {
    {2048, 0, 0, false}, {2048, 60, 0, true},
    {0, 70, 0, true}, {4095, 89, 0, true},
    {4095, 90, 0, true}, {4095, 108, 0, true}, {4095, 109, 1, false}
  });

  trace("center jitter restarts rearm dwell", {
    {2048, 0, 0, false}, {2048, 60, 0, true},
    {0, 70, 0, true}, {0, 90, -1, false},
    {2048, 100, 0, false}, {2399, 159, 0, false},
    {2048, 160, 0, false}, {1697, 219, 0, false},
    {1698, 220, 0, false}, {2398, 279, 0, false}, {2048, 280, 0, true},
    {4095, 290, 0, true}, {4095, 310, 1, false}
  });

  trace("direct traverse and brief center cannot produce a second shift", {
    {2048, 0, 0, false}, {2048, 60, 0, true},
    {4095, 70, 0, true}, {4095, 90, 1, false},
    {0, 100, 0, false}, {0, 1000, 0, false},
    {2048, 1010, 0, false}, {0, 1069, 0, false}, {0, 2000, 0, false},
    {2048, 2010, 0, false}, {2048, 2070, 0, true},
    {0, 2080, 0, true}, {0, 2100, -1, false}
  });

  trace("threshold boundaries and intermediate travel", {
    {1698, 0, 0, false}, {2398, 60, 0, true},
    {3047, 70, 0, true}, {3047, 170, 0, true},
    {3048, 180, 0, true}, {3048, 200, 1, false},
    {2700, 210, 0, false}, {2700, 1000, 0, false},
    {2048, 1010, 0, false}, {2048, 1070, 0, true},
    {1049, 1080, 0, true}, {1049, 1180, 0, true},
    {1048, 1190, 0, true}, {1048, 1210, -1, false}
  });

  StickShiftConfig reversed;
  reversed.inverted = true;
  trace("inversion reverses both gear directions", {
    {2048, 0, 0, false}, {2048, 60, 0, true},
    {4095, 70, 0, true}, {4095, 90, -1, false},
    {2048, 100, 0, false}, {2048, 160, 0, true},
    {0, 170, 0, true}, {0, 190, 1, false}
  }, reversed);

  StickShiftConfig calibrated;
  calibrated.center = 1800;
  calibrated.triggerDelta = 800;
  calibrated.centerDeadzone = 200;
  calibrated.centerHoldMs = 30;
  calibrated.triggerHoldMs = 10;
  trace("board calibration and debounce durations are configurable", {
    {1600, 0, 0, false}, {2000, 29, 0, false}, {1800, 30, 0, true},
    {2600, 40, 0, true}, {2600, 49, 0, true}, {2600, 50, 1, false},
    {1800, 60, 0, false}, {1800, 90, 0, true},
    {1000, 100, 0, true}, {1000, 110, -1, false}
  }, calibrated);

  const uint32_t last = std::numeric_limits<uint32_t>::max();
  trace("center dwell survives millis wrap", {
    {2048, last - 39, 0, false}, {2048, 19, 0, false}, {2048, 20, 0, true},
    {4095, 30, 0, true}, {4095, 50, 1, false}
  });
  trace("trigger debounce survives millis wrap", {
    {2048, last - 100, 0, false}, {2048, last - 40, 0, true},
    {0, last - 9, 0, true}, {0, 9, 0, true}, {0, 10, -1, false}
  });

  ++scenarios;
  StickShift reset;
  reset.update(2048, 0);
  reset.update(2048, 60);
  reset.update(4095, 70);
  reset.reset();
  expect(!reset.armed() && reset.update(4095, 90) == 0,
         "reset cancels a pending outer debounce");
  expect(reset.update(4095, 1000) == 0, "reset cannot shift a held stick");
  reset.update(2048, 1010);
  reset.reset();
  reset.update(2048, 1060);
  expect(!reset.armed(), "reset discards partial center dwell");
  expect(reset.update(2048, 1119) == 0 && !reset.armed(), "fresh center dwell is required");
  expect(reset.update(2048, 1120) == 0 && reset.armed(), "fresh center dwell rearms after reset");
  reset.reset();
  expect(!reset.armed(), "reset disarms an already armed stick");

  if (failures) {
    std::fprintf(stderr, "%d failures in %d scenarios\n", failures, scenarios);
    return 1;
  }
  std::printf("PASS: %d thumbstick scenarios\n", scenarios);
  return 0;
}
