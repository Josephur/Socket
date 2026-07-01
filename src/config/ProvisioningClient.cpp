#include "ProvisioningClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "../config/DeviceIdentity.h"
#include "../core/Logger.h"

namespace ProvisioningClient {

bool fetch(const String &provisioningUrl, RuntimeConfig &outConfig) {
  static const char *kTag = "Provisioning";

  String url = provisioningUrl + "?device_id=" + DeviceIdentity::chipId();

  HTTPClient http;
  if (!http.begin(url)) {
    Logger::error(kTag, "HTTPClient::begin failed");
    return false;
  }

  int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Logger::error(kTag, ("GET failed: " + String(status)).c_str());
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Logger::error(kTag, ("JSON parse failed: " + String(err.c_str())).c_str());
    return false;
  }

  outConfig.deviceName = doc["device_name"] | "";
  outConfig.persona = doc["persona"] | "";
  outConfig.wakeWord = doc["wake_word"] | "";
  outConfig.ttsVoice = doc["tts_voice"] | "";
  outConfig.converseEndpoint = doc["converse_endpoint"] | "";
  outConfig.loaded = outConfig.converseEndpoint.length() > 0;

  return outConfig.loaded;
}

}  // namespace ProvisioningClient
