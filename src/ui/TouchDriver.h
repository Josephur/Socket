#pragma once

#include <cstdint>

// Wraps the vendored GT911 driver (src/ui/board/gt911.*). The board has no
// INT line wired (see docs/PINOUT.md), so touch state must be polled.
class TouchDriver {
 public:
  bool begin();

  // Polls the controller. Returns true if at least one point is pressed,
  // and writes the primary touch point's coordinates to x/y.
  bool read(uint16_t &x, uint16_t &y);

 private:
  void *touchHandle_ = nullptr;  // esp_lcd_touch_handle_t, opaque here to avoid pulling IDF headers into callers
};
