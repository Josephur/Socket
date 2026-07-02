#pragma once

#include <cstdint>

// Wraps the vendored GT911 driver (src/ui/board/gt911.*). The board has no
// INT line wired (see docs/PINOUT.md), so touch state must be polled.
class TouchDriver {
 public:
  bool begin();

  // Polls the controller. Returns true if at least one point is pressed,
  // and writes the primary touch point's coordinates to x/y.
  //
  // Safe to call from multiple pollers (loop() and LVGL's indev callback
  // both do): the last real hardware sample is cached here, so a read that
  // lands between GT911 scan cycles (~10ms apart) -- or after another
  // poller already consumed the sample -- reports the held state instead
  // of a spurious release.
  bool read(uint16_t &x, uint16_t &y);

 private:
  void *touchHandle_ = nullptr;  // esp_lcd_touch_handle_t, opaque here to avoid pulling IDF headers into callers

  // Last state from a fresh GT911 sample -- see read().
  bool pressed_ = false;
  uint16_t x_ = 0;
  uint16_t y_ = 0;
};
