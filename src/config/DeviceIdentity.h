#pragma once

#include <Arduino.h>

namespace DeviceIdentity {

// Stable per-device ID derived from the ESP32-P4's efuse MAC, e.g.
// "a1b2c3d4e5f6". Used to key the provisioning request so the server can
// tell devices apart without any manual per-unit setup step.
String chipId();

}  // namespace DeviceIdentity
