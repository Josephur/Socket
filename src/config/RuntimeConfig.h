#pragma once

#include <Arduino.h>

// Populated once at boot from the PHP provisioning endpoint's JSON response.
// Fields are deliberately flat and persona-scoped -- see the plan's
// "Fleet & persona" decision: config is keyed by device ID server-side, but
// the firmware just receives whatever this device's persona resolves to.
struct RuntimeConfig {
  String deviceName;
  String persona;
  String wakeWord;
  String ttsVoice;
  String converseEndpoint;  // Full URL for POST /api/converse-style calls.

  bool loaded = false;
};
