#include "Logger.h"

#include <Arduino.h>

namespace Logger {

void begin(unsigned long baud) {
  Serial.begin(baud);
}

static void log(const char *level, const char *tag, const char *message) {
  Serial.printf("[%s] %s: %s\n", level, tag, message);
  Serial.flush();
}

void info(const char *tag, const char *message) { log("INFO", tag, message); }
void warn(const char *tag, const char *message) { log("WARN", tag, message); }
void error(const char *tag, const char *message) {
  log("ERROR", tag, message);
}

void flush() { Serial.flush(); }

}  // namespace Logger
