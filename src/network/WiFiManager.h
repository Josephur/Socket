#pragma once

#include <Arduino.h>

// Credentials live in NVS (Preferences), not source, so re-flashing never
// wipes them and nothing shop-network-specific ends up in git. Connectivity
// is provided by the onboard ESP32-C6 co-processor over SDIO via ESP-Hosted,
// but from this code's point of view it's just the normal WiFi.h API.
class WiFiManager {
 public:
  // Loads credentials from NVS (namespace "socket-wifi") and attempts to
  // connect, blocking up to timeoutMs. Returns true if connected.
  bool begin(uint32_t timeoutMs = 15000);

  // Persists new credentials to NVS and reconnects. Use this from a future
  // setup flow (e.g. a captive portal); no such flow exists yet.
  bool setCredentials(const String &ssid, const String &password);

  bool isConnected() const;

  // Attempts a reconnect if not currently connected. Call periodically from
  // the OFFLINE state's handling in Socket.ino. Returns true once connected.
  bool reconnect(uint32_t timeoutMs = 10000);

 private:
  bool connectWithStoredCredentials(uint32_t timeoutMs);
};
