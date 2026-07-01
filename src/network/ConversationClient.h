#pragma once

#include <Arduino.h>

// POSTs one recorded utterance to the PHP orchestration endpoint
// (RuntimeConfig::converseEndpoint) and gets back the LLM's transcript plus
// synthesized TTS audio in a single response -- see the plan's "Transport"
// decision (REST per-utterance, no WebSocket).
//
// PROPOSED wire format for the response body (needs to be agreed with the
// PHP side before this is load-bearing -- nothing server-side exists yet):
//   [4 bytes big-endian uint32: JSON header length N]
//   [N bytes: JSON header, e.g. {"transcript": "...", "audio_format": "wav"}]
//   [remaining bytes: raw TTS audio, format as named in the header]
// A length-prefixed binary framing avoids base64's ~33% size overhead for
// the audio payload, which matters for repeated round trips on this device.
//
// The whole response is buffered in PSRAM rather than streamed to the I2S
// output as it arrives -- simpler, and this board has 32MB of PSRAM to
// spare. Revisit only if response sizes or latency become a problem.
struct ConversationResult {
  String transcript;
  String audioFormat;
  uint8_t *audioData = nullptr;  // Caller must free() this.
  size_t audioLen = 0;
  bool ok = false;
};

namespace ConversationClient {

// `utteranceAudio`/`utteranceLen` is the recorded mic audio (format TBD
// alongside AudioRecorder -- see its header). Blocks until the full
// response is received or the request fails/times out.
ConversationResult converse(const String &converseEndpoint,
                             const uint8_t *utteranceAudio,
                             size_t utteranceLen);

}  // namespace ConversationClient
