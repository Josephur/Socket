#pragma once

#include <Arduino.h>

#include "AudioCodec.h"

// STATUS: unresolved research spike (see the plan's "Key Risks" #2). There
// is no confirmed off-the-shelf Arduino wake-word library for the ESP32-P4
// today -- Espressif's ESP-SR is ESP-IDF-first, and its P4 support isn't
// established. Options to evaluate before this is real:
//   1. Port/wrap ESP-SR directly (it's IDF C, callable from Arduino sketches
//      the same way es8311 would be -- see AudioCodec.h for that pattern).
//   2. A much simpler energy-threshold VAD (not true wake-word) as a
//      fallback, if a wake-word engine proves infeasible in-timeframe.
//   3. Fall back to touch-button activation (the runner-up option from the
//      "Activation" decision) if neither of the above pans out.
// This class is a placeholder so AppState/Socket.ino can be wired end-to-end
// against a stable interface while that spike happens.
class WakeWordDetector {
 public:
  explicit WakeWordDetector(AudioCodec &codec) : codec_(codec) {}

  bool begin(const String &wakeWord) { return false; }  // Not implemented.

  // Should return true exactly once when the wake word is detected. Always
  // returns false until this is implemented.
  bool checkForWakeWord() { return false; }

 private:
  AudioCodec &codec_;
};
