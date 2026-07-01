#pragma once

#include <Arduino.h>

// Not implemented in v1 (see plan: "OTA firmware updates -- architecture
// leaves room, not required to be functional in v1"). This exists so the
// eventual implementation has an obvious home and Socket.ino's structure
// doesn't need to change when it's built.
namespace OtaUpdater {

// TODO: check `checkUrl` for a newer firmware binary and flash it via
// Arduino's Update.h if found. Needs a versioning scheme agreed with the
// PHP server side first.
bool checkAndApply(const String &checkUrl);

}  // namespace OtaUpdater
