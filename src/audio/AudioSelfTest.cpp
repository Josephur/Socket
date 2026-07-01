#include "AudioSelfTest.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <math.h>

#include "../core/Logger.h"

namespace AudioSelfTest {

namespace {
constexpr const char *kTag = "AudioSelfTest";
constexpr size_t kChunkSamples = 512;
constexpr float kToneFrequencyHz = 1000.0f;
constexpr float kToneAmplitude = 12000.0f;  // out of +/-32767, avoid clipping

float rms(const int16_t *samples, size_t count) {
  if (count == 0) return 0.0f;
  double sumSquares = 0.0;
  for (size_t i = 0; i < count; i++) {
    sumSquares += static_cast<double>(samples[i]) * samples[i];
  }
  return static_cast<float>(sqrt(sumSquares / count));
}

void fillSineChunk(int16_t *buf, size_t count, uint32_t sampleRateHz,
                    float freqHz, float &phase) {
  float phaseStep = 2.0f * PI * freqHz / sampleRateHz;
  for (size_t i = 0; i < count; i++) {
    buf[i] = static_cast<int16_t>(kToneAmplitude * sinf(phase));
    phase += phaseStep;
    if (phase > 2.0f * PI) phase -= 2.0f * PI;
  }
}
}  // namespace

ToneTestResult runToneDetectionTest(AudioCodec &codec, uint32_t sampleRateHz) {
  ToneTestResult result;
  int16_t chunk[kChunkSamples];

  Logger::info(kTag, "measuring baseline silence RMS...");
  double baselineSum = 0.0;
  constexpr int kBaselineChunks = 10;
  for (int i = 0; i < kBaselineChunks; i++) {
    codec.read(chunk, kChunkSamples);
    baselineSum += rms(chunk, kChunkSamples);
  }
  result.baselineRms = static_cast<float>(baselineSum / kBaselineChunks);
  Logger::info(kTag,
               ("baseline RMS: " + String(result.baselineRms)).c_str());

  Logger::info(kTag, "playing 1kHz tone and recording simultaneously...");
  int16_t toneChunk[kChunkSamples];
  float phase = 0.0f;
  double toneSum = 0.0;
  constexpr int kToneChunks = 40;  // ~40 * 512 samples @ 24kHz =~ 0.85s
  for (int i = 0; i < kToneChunks; i++) {
    fillSineChunk(toneChunk, kChunkSamples, sampleRateHz, kToneFrequencyHz,
                  phase);
    codec.write(toneChunk, kChunkSamples);
    codec.read(chunk, kChunkSamples);
    toneSum += rms(chunk, kChunkSamples);
  }
  result.toneRms = static_cast<float>(toneSum / kToneChunks);
  Logger::info(kTag, ("tone-playing RMS: " + String(result.toneRms)).c_str());

  // Require the tone-playing RMS to meaningfully exceed the silence
  // baseline -- not just any nonzero reading, since ADC noise floor alone
  // can produce small nonzero RMS. Thresholds are a rough sanity check, not
  // a calibrated measurement.
  result.toneDetected =
      result.toneRms > (result.baselineRms * 1.5f + 50.0f);

  Logger::info(kTag, result.toneDetected
                          ? "TONE DETECTED -- speaker+mic path appears to "
                            "be physically working"
                          : "no clear tone detected -- see AudioSelfTest.h; "
                            "could be codec register tuning or a physical "
                            "connection issue, needs the manual listening "
                            "test to tell which");
  return result;
}

void runLiveLoopbackForever(AudioCodec &codec) {
  Logger::info(kTag, "entering live mic->speaker loopback -- talk into the "
                      "mic and listen for it on the speaker");
  int16_t chunk[kChunkSamples];
  uint32_t lastLogMs = 0;
  while (true) {
    size_t read = codec.read(chunk, kChunkSamples);
    codec.write(chunk, read);

    if (millis() - lastLogMs > 2000) {
      lastLogMs = millis();
      Logger::info(kTag,
                   ("loopback running, last chunk RMS: " +
                    String(rms(chunk, read)))
                       .c_str());
    }
  }
}

void playTestTone(AudioCodec &codec, uint32_t sampleRateHz,
                   uint32_t durationMs, float freqHz) {
  Logger::info(kTag, ("playing test tone: " + String(freqHz) + "Hz for " +
                       String(durationMs) + "ms")
                          .c_str());
  int16_t chunk[kChunkSamples];
  float phase = 0.0f;
  uint32_t totalSamples = sampleRateHz * durationMs / 1000;
  uint32_t samplesWritten = 0;
  while (samplesWritten < totalSamples) {
    size_t thisChunk = min((uint32_t)kChunkSamples, totalSamples - samplesWritten);
    fillSineChunk(chunk, thisChunk, sampleRateHz, freqHz, phase);
    codec.write(chunk, thisChunk);
    samplesWritten += thisChunk;
  }
}

void playBeeps(AudioCodec &codec, int count, uint32_t sampleRateHz,
               uint32_t beepMs, uint32_t gapMs, float freqHz) {
  int16_t silence[kChunkSamples] = {0};
  for (int i = 0; i < count; i++) {
    if (i > 0) {
      uint32_t gapSamples = sampleRateHz * gapMs / 1000;
      uint32_t written = 0;
      while (written < gapSamples) {
        size_t thisChunk = min((uint32_t)kChunkSamples, gapSamples - written);
        codec.write(silence, thisChunk);
        written += thisChunk;
      }
    }
    playTestTone(codec, sampleRateHz, beepMs, freqHz);
  }
}

void runRecordPlaybackDemo(AudioCodec &codec, uint32_t sampleRateHz,
                            uint32_t recordSeconds) {
  Logger::info(kTag, "record/playback demo: 3 beeps...");
  playBeeps(codec, 3, sampleRateHz);

  size_t totalSamples = sampleRateHz * recordSeconds;
  auto *recordBuf = static_cast<int16_t *>(
      heap_caps_malloc(totalSamples * sizeof(int16_t), MALLOC_CAP_SPIRAM));
  if (!recordBuf) {
    Logger::error(kTag, "PSRAM allocation for record buffer failed");
    return;
  }

  Logger::info(kTag, ("recording for " + String(recordSeconds) +
                       "s -- talk now")
                          .c_str());
  size_t recorded = 0;
  while (recorded < totalSamples) {
    size_t toRead = min((size_t)kChunkSamples, totalSamples - recorded);
    recorded += codec.read(recordBuf + recorded, toRead);
  }
  Logger::info(kTag, ("recording done, RMS: " + String(rms(recordBuf,
                                                             totalSamples)))
                          .c_str());

  Logger::info(kTag, "3 beeps...");
  playBeeps(codec, 3, sampleRateHz);

  Logger::info(kTag, "playing back the recording...");
  size_t written = 0;
  while (written < totalSamples) {
    size_t thisChunk = min((size_t)kChunkSamples, totalSamples - written);
    codec.write(recordBuf + written, thisChunk);
    written += thisChunk;
  }

  heap_caps_free(recordBuf);
  Logger::info(kTag, "record/playback demo complete");
}

}  // namespace AudioSelfTest
