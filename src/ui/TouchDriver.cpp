#include "TouchDriver.h"

#include "../core/Logger.h"
#include "board/SharedI2CBus.h"
#include "board/gt911.h"
#include "board/touch.h"

namespace {
constexpr const char *kTag = "TouchDriver";
}

bool TouchDriver::begin() {
  Logger::info(kTag, "begin() start -- acquiring shared I2C bus");
  Logger::flush();
  DEV_I2C_Port &bus = SharedI2CBus();

  // touch_gt911_init (vendored, src/ui/board/gt911.cpp) uses
  // ESP_ERROR_CHECK internally on the I2C reads it does to identify the
  // controller -- if the GT911 doesn't ACK (wrong address, not powered,
  // wiring issue), ESP_ERROR_CHECK calls abort() and the whole chip
  // resets right here, with no chance for this function to return or log
  // anything further. If you see "begin() start" in the serial log
  // followed immediately by a reboot with nothing in between, this call is
  // the crash site -- flushing first so that line survives.
  Logger::info(kTag, "calling touch_gt911_init() -- can abort() on I2C NACK");
  Logger::flush();
  touchHandle_ = touch_gt911_init(bus);

  Logger::info(kTag, touchHandle_ ? "begin() succeeded"
                                   : "begin() failed (null handle)");
  return touchHandle_ != nullptr;
}

bool TouchDriver::read(uint16_t &x, uint16_t &y) {
  if (!touchHandle_) return false;

  auto tp = static_cast<esp_lcd_touch_handle_t>(touchHandle_);

  esp_lcd_touch_read_data(tp);

  // Only a fresh GT911 sample may change the cached state. The controller
  // scans at ~100Hz while this can be polled every ~5ms from loop() AND
  // every ~33ms from LVGL's indev callback; get_coordinates() zeroes the
  // driver's point buffer on every call, so without this gate whichever
  // poller reads second (or lands between scan cycles) would see a phantom
  // release mid-press -- the cause of missed taps and a skipping cursor.
  if (touch_gt911_last_read_fresh()) {
    static constexpr uint8_t kMaxPoints = 5;
    uint16_t xs[kMaxPoints] = {0};
    uint16_t ys[kMaxPoints] = {0};
    uint16_t strength[kMaxPoints] = {0};
    uint8_t count = 0;
    bool pressed = esp_lcd_touch_get_coordinates(tp, xs, ys, strength, &count,
                                                  kMaxPoints);
    pressed_ = pressed && count > 0;
    if (pressed_) {
      x_ = xs[0];
      y_ = ys[0];
    }
  }

  if (pressed_) {
    x = x_;
    y = y_;
  }
  return pressed_;
}
