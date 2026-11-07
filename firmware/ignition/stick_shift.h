#pragma once
#include <cstdint>

namespace ignition {
struct StickShiftConfig {
  // ADC calibration; triggerDelta must exceed centerDeadzone >= 0.
  int center = 2048;
  int triggerDelta = 1000;
  int centerDeadzone = 350;
  bool inverted = false;
  uint32_t centerHoldMs = 60;
  uint32_t triggerHoldMs = 20;
};

class StickShift {
  const StickShiftConfig config_;
  bool armed_ = false;
  bool centering_ = false;
  int candidate_ = 0;
  uint32_t centerSince_ = 0;
  uint32_t triggerSince_ = 0;

public:
  explicit StickShift(const StickShiftConfig& config = StickShiftConfig()) : config_(config) {}

  // Call regularly with the current ADC sample and millis(). Returns one shift
  // per excursion: +1 for high ADC/right, -1 for low ADC/left, or 0. Inversion
  // changes event direction only. Time subtraction remains valid across wrap.
  int update(int raw, uint32_t now) {
    const int64_t delta = int64_t(raw) - config_.center;
    const bool centered = delta >= -int64_t(config_.centerDeadzone)
                       && delta <= config_.centerDeadzone;
    if (!armed_) {
      if (!centered) {
        centering_ = false;
      } else {
        if (!centering_) {
          centering_ = true;
          centerSince_ = now;
        }
        if (uint32_t(now - centerSince_) >= config_.centerHoldMs) {
          armed_ = true;
          centering_ = false;
        }
      }
      return 0;
    }

    const int direction = delta >= config_.triggerDelta ? 1
                        : delta <= -int64_t(config_.triggerDelta) ? -1 : 0;
    if (!direction) {
      candidate_ = 0;
      return 0;
    }
    if (direction != candidate_) {
      candidate_ = direction;
      triggerSince_ = now;
    }
    if (uint32_t(now - triggerSince_) < config_.triggerHoldMs) return 0;

    reset(); // Held, bouncing, or directly reversed sticks cannot repeat.
    return config_.inverted ? -direction : direction;
  }

  bool armed() const { return armed_; }

  // Use when controls are inhibited (engine off or diagnostic mode). Calling
  // update afterward requires a new full center dwell, even after a long hold.
  void reset() {
    armed_ = false;
    centering_ = false;
    candidate_ = 0;
    centerSince_ = triggerSince_ = 0;
  }
};
} // namespace ignition
