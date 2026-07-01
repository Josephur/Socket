#include "DisplayDriver.h"

#include "../core/Logger.h"
#include "board/displays_config.h"

namespace {
constexpr const char *kTag = "DisplayDriver";
}

// GPIO26 backlight is wired inverted (LOW = on) per docs/PINOUT.md.
static constexpr int kBacklightPin = 26;

bool DisplayDriver::begin() {
#ifndef BOARD_HAS_PSRAM
#error "DisplayDriver requires PSRAM enabled (Tools > PSRAM in Arduino IDE)"
#endif

  Logger::info(kTag, "begin() start");

  pinMode(kBacklightPin, OUTPUT);
  setBacklight(false);

  Logger::info(kTag, "constructing Arduino_ESP32DSIPanel");
  Logger::flush();
  auto *dsiPanel = new Arduino_ESP32DSIPanel(
      display_cfg.hsync_pulse_width, display_cfg.hsync_back_porch,
      display_cfg.hsync_front_porch, display_cfg.vsync_pulse_width,
      display_cfg.vsync_back_porch, display_cfg.vsync_front_porch,
      display_cfg.prefer_speed, display_cfg.lane_bit_rate);

  Logger::info(kTag, "constructing Arduino_DSI_Display");
  Logger::flush();
  gfx_ = new Arduino_DSI_Display(
      display_cfg.width, display_cfg.height, dsiPanel, 0, true,
      display_cfg.lcd_rst, display_cfg.init_cmds, display_cfg.init_cmds_size);

  Logger::info(kTag, "calling gfx->begin() -- MIPI DSI panel init");
  Logger::flush();
  if (!gfx_->begin()) {
    Logger::error(kTag, "gfx->begin() returned false");
    return false;
  }
  Logger::info(kTag, "gfx->begin() succeeded");

  setBacklight(true);
  Logger::info(kTag, "begin() complete, backlight on");
  return true;
}

void DisplayDriver::setBacklight(bool on) {
  // Inverted: LOW turns the backlight on.
  digitalWrite(kBacklightPin, on ? LOW : HIGH);
}

uint16_t DisplayDriver::width() const { return display_cfg.width; }
uint16_t DisplayDriver::height() const { return display_cfg.height; }
