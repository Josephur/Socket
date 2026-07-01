#pragma once

#include "AudioCodec.h"

// Speaker/mic bring-up test harness. Two modes:
//
//  1. runToneDetectionTest(): plays a 1kHz tone out the speaker while
//     simultaneously recording from the mic, and compares recorded loudness
//     against a pre-tone silence baseline. This is an automated sanity
//     check that doesn't require a human to listen -- if the mic picks up
//     the tone (even faintly, even distorted), that's real evidence the
//     physical speaker->air->mic path and both codecs' I2C/I2S plumbing are
//     working, regardless of whether the register-level tuning in
//     Es8311/Es7210 is perfect.
//
//  2. runLiveLoopbackForever(): continuous mic -> speaker passthrough, for
//     a human to test by talking into the mic and listening to the
//     speaker. Never returns -- meant to be the whole content of loop() in
//     a dedicated test build (see Socket.ino's SOCKET_AUDIO_TEST_MODE).
namespace AudioSelfTest {

struct ToneTestResult {
  float baselineRms = 0.0f;
  float toneRms = 0.0f;
  bool toneDetected = false;
};

ToneTestResult runToneDetectionTest(AudioCodec &codec,
                                     uint32_t sampleRateHz = 24000);

[[noreturn]] void runLiveLoopbackForever(AudioCodec &codec);

}  // namespace AudioSelfTest
