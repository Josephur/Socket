#include "Logger.h"

#include <Arduino.h>

namespace Logger {

void begin(unsigned long baud) {
  // Big enough for several ScreenshotService lines (~140 bytes each, see
  // its availableForWrite() gate -- with only the default buffer/FIFO that
  // gate could never open) and to keep bursts of log lines from blocking
  // the caller at 115200 baud. Must be set before Serial.begin().
  Serial.setTxBufferSize(2048);
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
