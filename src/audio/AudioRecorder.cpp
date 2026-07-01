#include "AudioRecorder.h"

#include <esp_heap_caps.h>

#include "../core/Logger.h"

int16_t *AudioRecorder::recordUtterance(size_t &outSampleCount,
                                         uint32_t maxDurationMs,
                                         uint32_t silenceTimeoutMs) {
  // TODO: replace this fixed-duration capture with real silence-based
  // endpointing once AudioCodec has a working codec (see AudioCodec.h) to
  // validate against real audio. For now this always records the full
  // maxDurationMs window.
  outSampleCount = 0;

  constexpr uint32_t kSampleRateHz = 24000;
  size_t maxSamples = (size_t)kSampleRateHz * maxDurationMs / 1000;

  auto *buffer = static_cast<int16_t *>(
      heap_caps_malloc(maxSamples * sizeof(int16_t), MALLOC_CAP_SPIRAM));
  if (!buffer) {
    Logger::error("AudioRecorder", "PSRAM allocation failed");
    return nullptr;
  }

  size_t read = codec_.read(buffer, maxSamples);
  outSampleCount = read;
  return buffer;
}
