#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

#include "AudioCodec.h"

// Plays back the audio returned by ConversationClient::converse(). v1 keeps
// this to WAV/PCM only, matching the "audio_format" field in the proposed
// response header -- see ConversationClient.h.
class AudioPlayer {
 public:
  explicit AudioPlayer(AudioCodec &codec) : codec_(codec) {}

  // Blocks until playback completes. `format` currently only supports "wav"
  // (a standard RIFF/WAVE PCM file) -- returns false for anything else.
  bool play(const uint8_t *audioData, size_t audioLen, const String &format);

 private:
  AudioCodec &codec_;
};
