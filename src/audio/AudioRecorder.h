#pragma once

#include <cstddef>
#include <cstdint>

#include "AudioCodec.h"

// Captures one utterance into a PSRAM buffer after the wake word fires,
// stopping on silence or a max-duration cap. Kept deliberately simple for
// v1: no streaming, no VAD-based endpointing beyond a basic silence timeout
// -- see WakeWordDetector.h for where real VAD logic should eventually live.
class AudioRecorder {
 public:
  explicit AudioRecorder(AudioCodec &codec) : codec_(codec) {}

  // Records until `maxDurationMs` elapses or `silenceTimeoutMs` of
  // near-silence is observed after speech starts. Returns a PSRAM-allocated
  // 16-bit PCM buffer (caller must free()) and its sample count via
  // outSampleCount, or nullptr on failure.
  int16_t *recordUtterance(size_t &outSampleCount,
                            uint32_t maxDurationMs = 10000,
                            uint32_t silenceTimeoutMs = 1500);

 private:
  AudioCodec &codec_;
};
