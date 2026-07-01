#pragma once

#include <lvgl.h>

#include "DisplayDriver.h"
#include "TouchDriver.h"

// Wires DisplayDriver + TouchDriver into LVGL: draw buffers, flush callback,
// touchpad input device, and the tick timer. Ported from the proven
// docs/Arduino/examples/LVGLV9_Arduino pattern.
class LvglBridge {
 public:
  // Call after DisplayDriver::begin() and TouchDriver::begin() both succeed.
  bool begin(DisplayDriver &display, TouchDriver &touch);

  // Call every loop() iteration to pump LVGL's timer handler.
  void tick();

 private:
  static void flushCallback(lv_display_t *disp, const lv_area_t *area,
                             uint8_t *pixelMap);
  static void touchReadCallback(lv_indev_t *indev, lv_indev_data_t *data);
  static void tickTimerCallback(void *arg);

  DisplayDriver *display_ = nullptr;
  TouchDriver *touch_ = nullptr;
  lv_display_t *lvDisplay_ = nullptr;
  lv_indev_t *lvTouchpad_ = nullptr;
  lv_color_t *drawBuf1_ = nullptr;
  lv_color_t *drawBuf2_ = nullptr;
};
