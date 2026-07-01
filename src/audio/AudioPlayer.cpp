#include "AudioPlayer.h"

#include <Arduino.h>
#include <cstring>

#include "../core/Logger.h"

namespace {
// Minimal RIFF/WAVE header parser: finds the "data" chunk and returns a
// pointer/length to the raw PCM samples within audioData. Assumes 16-bit
// PCM, which is what F5-TTS/XTTSv2 (docs/TTS_ENDPOINTS.md) produce.
bool findWavDataChunk(const uint8_t *audioData, size_t audioLen,
                      const int16_t *&outSamples, size_t &outSampleCount) {
  if (audioLen < 44) return false;  // Smaller than a minimal WAV header.

  size_t pos = 12;  // Skip "RIFF"+size+"WAVE".
  while (pos + 8 <= audioLen) {
    char chunkId[5] = {0};
    memcpy(chunkId, audioData + pos, 4);
    uint32_t chunkSize = audioData[pos + 4] | (audioData[pos + 5] << 8) |
                         (audioData[pos + 6] << 16) |
                         (audioData[pos + 7] << 24);
    size_t dataStart = pos + 8;

    if (strcmp(chunkId, "data") == 0) {
      if (dataStart + chunkSize > audioLen) chunkSize = audioLen - dataStart;
      outSamples = reinterpret_cast<const int16_t *>(audioData + dataStart);
      outSampleCount = chunkSize / sizeof(int16_t);
      return true;
    }
    pos = dataStart + chunkSize + (chunkSize % 2);  // Chunks are word-aligned.
  }
  return false;
}
}  // namespace

bool AudioPlayer::play(const uint8_t *audioData, size_t audioLen,
                       const String &format) {
  if (format != "wav") {
    Logger::error("AudioPlayer",
                   ("Unsupported audio format: " + format).c_str());
    return false;
  }

  const int16_t *samples = nullptr;
  size_t sampleCount = 0;
  if (!findWavDataChunk(audioData, audioLen, samples, sampleCount)) {
    Logger::error("AudioPlayer", "Failed to locate WAV data chunk");
    return false;
  }

  codec_.write(samples, sampleCount);
  return true;
}
