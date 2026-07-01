#include "ConversationClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>

#include "../core/Logger.h"

namespace ConversationClient {

namespace {
constexpr const char *kTag = "Conversation";
}

ConversationResult converse(const String &converseEndpoint,
                             const uint8_t *utteranceAudio,
                             size_t utteranceLen) {
  ConversationResult result;

  HTTPClient http;
  if (!http.begin(converseEndpoint)) {
    Logger::error(kTag, "HTTPClient::begin failed");
    return result;
  }
  http.addHeader("Content-Type", "application/octet-stream");

  int status = http.POST(const_cast<uint8_t *>(utteranceAudio), utteranceLen);
  if (status != HTTP_CODE_OK) {
    Logger::error(kTag, ("POST failed: " + String(status)).c_str());
    http.end();
    return result;
  }

  auto *stream = http.getStreamPtr();

  uint8_t lenBytes[4];
  if (stream->readBytes(lenBytes, 4) != 4) {
    Logger::error(kTag, "Failed to read header length prefix");
    http.end();
    return result;
  }
  uint32_t headerLen = (uint32_t(lenBytes[0]) << 24) |
                        (uint32_t(lenBytes[1]) << 16) |
                        (uint32_t(lenBytes[2]) << 8) | uint32_t(lenBytes[3]);

  String headerJson;
  headerJson.reserve(headerLen);
  for (uint32_t i = 0; i < headerLen; i++) {
    while (!stream->available()) {
      if (!http.connected()) {
        Logger::error(kTag, "Connection dropped reading header");
        http.end();
        return result;
      }
      delay(1);
    }
    headerJson += (char)stream->read();
  }

  JsonDocument doc;
  if (deserializeJson(doc, headerJson)) {
    Logger::error(kTag, "Failed to parse response header JSON");
    http.end();
    return result;
  }
  result.transcript = doc["transcript"] | "";
  result.audioFormat = doc["audio_format"] | "wav";

  int remaining = http.getSize() - 4 - headerLen;
  if (remaining < 0) remaining = 0;

  result.audioData =
      static_cast<uint8_t *>(heap_caps_malloc(remaining, MALLOC_CAP_SPIRAM));
  if (!result.audioData) {
    Logger::error(kTag, "PSRAM allocation for audio payload failed");
    http.end();
    return result;
  }

  size_t readTotal = 0;
  while (readTotal < (size_t)remaining && http.connected()) {
    if (stream->available()) {
      readTotal += stream->readBytes(result.audioData + readTotal,
                                      remaining - readTotal);
    } else {
      delay(1);
    }
  }
  result.audioLen = readTotal;
  result.ok = (readTotal == (size_t)remaining);

  http.end();
  return result;
}

}  // namespace ConversationClient
