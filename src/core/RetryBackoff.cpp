#include "RetryBackoff.h"

#include <Arduino.h>

constexpr uint32_t RetryBackoff::kIntervalsSec[];

void RetryBackoff::reset() {
  stage_ = 0;
  lastAttemptMs_ = millis();
}

bool RetryBackoff::dueNow() {
  uint32_t intervalMs = kIntervalsSec[stage_] * 1000UL;
  if (millis() - lastAttemptMs_ < intervalMs) return false;

  lastAttemptMs_ = millis();
  if (stage_ < kNumIntervals - 1) stage_++;
  return true;
}
