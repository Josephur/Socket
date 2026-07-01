#include "WiFiManager.h"

#include <Preferences.h>
#include <WiFi.h>

#include "../core/Logger.h"

namespace {
constexpr const char *kNamespace = "socket-wifi";
constexpr const char *kTag = "WiFiManager";
}  // namespace

bool WiFiManager::begin(uint32_t timeoutMs) {
  return connectWithStoredCredentials(timeoutMs);
}

bool WiFiManager::setCredentials(const String &ssid, const String &password) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
    Logger::error(kTag, "Failed to open NVS namespace for write");
    return false;
  }
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
  prefs.end();

  return connectWithStoredCredentials(15000);
}

bool WiFiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::reconnect(uint32_t timeoutMs) {
  if (isConnected()) return true;
  return connectWithStoredCredentials(timeoutMs);
}

bool WiFiManager::connectWithStoredCredentials(uint32_t timeoutMs) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/true)) {
    Logger::error(kTag, "No stored WiFi credentials found in NVS");
    return false;
  }
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  prefs.end();

  if (ssid.isEmpty()) {
    Logger::error(kTag, "Stored SSID is empty");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
  }

  bool connected = WiFi.status() == WL_CONNECTED;
  Logger::info(kTag, connected ? "Connected" : "Connect timed out");
  return connected;
}
