#include "ActivityMonitor.h"

#include <Arduino.h>

namespace ActivityMonitor {

namespace {
uint32_t g_lastActivityMs = 0;
}

void notify() { g_lastActivityMs = millis(); }

uint32_t msSinceLastActivity() { return millis() - g_lastActivityMs; }

}  // namespace ActivityMonitor
