#pragma once

// Thin Serial logging wrapper so call sites don't sprinkle raw Serial.print
// calls, and so log formatting can change in one place later (timestamps,
// levels, a ring buffer for on-screen diagnostics, etc).

namespace Logger {

void begin(unsigned long baud = 115200);

void info(const char *tag, const char *message);
void warn(const char *tag, const char *message);
void error(const char *tag, const char *message);

// Blocks until the UART TX buffer is fully drained. Call this after logging
// immediately before anything that might crash/reset the chip (an
// ESP_ERROR_CHECK-guarded call, an untested driver path, etc) -- otherwise a
// crash can eat the last few log lines that were still sitting in the
// buffer, which is exactly the evidence you need to diagnose the crash.
void flush();

}  // namespace Logger
