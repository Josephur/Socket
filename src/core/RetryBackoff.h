#pragma once

#include <cstddef>
#include <cstdint>

// Staggered retry schedule: waits 3, 5, 10, 15, 30, then 60s between
// attempts, holding at 60s thereafter until reset(). Used to keep the
// OFFLINE state from hammering the WiFi stack or the provisioning endpoint
// on every loop() tick.
class RetryBackoff {
 public:
  RetryBackoff() { reset(); }

  // Call when entering a failure state (or right after a successful
  // attempt, so the *next* failure starts the schedule fresh from 3s).
  void reset();

  // Returns true once the current interval has elapsed since the last
  // reset()/true-returning call, and advances to the next (longer)
  // interval as a side effect. Cheap to call every loop() tick.
  bool dueNow();

 private:
  static constexpr uint32_t kIntervalsSec[] = {3, 5, 10, 15, 30, 60};
  static constexpr size_t kNumIntervals = 6;

  size_t stage_ = 0;
  uint32_t lastAttemptMs_ = 0;
};
