#pragma once

// Thin wrapper around the vendored Waveshare ST7703/MIPI-DSI init code
// (src/ui/board/displays_config.h) + Arduino_GFX. Ported from the proven
// docs/Arduino/examples/LVGLV9_Arduino example.

#include <Arduino_GFX_Library.h>

class DisplayDriver {
 public:
  // Initializes the DSI panel and backlight. Requires PSRAM enabled.
  // Returns false if the panel failed to initialize.
  bool begin();

  // Backlight is wired inverted on GPIO26 (LOW = on).
  void setBacklight(bool on);

  uint16_t width() const;
  uint16_t height() const;

  // Raw Arduino_GFX handle, needed by LvglBridge's flush callback.
  Arduino_GFX *gfx() const { return gfx_; }

  // Read access to the live DSI framebuffer (width*height RGB565 pixels in
  // PSRAM) -- what the panel is scanning out right now, all layers
  // composited. Used by ScreenshotService. Safe to cast: begin() always
  // constructs an Arduino_DSI_Display.
  uint16_t *framebuffer() const {
    return gfx_ ? static_cast<Arduino_DSI_Display *>(gfx_)->getFramebuffer()
                : nullptr;
  }

 private:
  Arduino_GFX *gfx_ = nullptr;
};
