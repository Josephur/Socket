#include "LvglBridge.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>

#include "../core/ActivityMonitor.h"
#include "../core/Logger.h"

namespace {
constexpr uint32_t kLvglTickPeriodMs = 5;
// Kept small deliberately: these buffers are MALLOC_CAP_DMA, which on this
// chip comes out of a small internal SRAM pool shared with the ESP-Hosted
// SDIO transport driver's own DMA mempool for the WiFi/BT link to the C6.
// At height=50 (144KB combined for both buffers) WiFi init reliably
// crashed with "HS_MP: mempool create failed: no mem" -- see
// docs/PROGRESS.md. Lower this further only if display performance is
// unacceptably slow; raise it only after confirming with
// heap_caps_get_free_size(MALLOC_CAP_DMA) that WiFi has already claimed
// its pool.
constexpr uint32_t kDrawBufHeight = 16;
constexpr const char *kTag = "LvglBridge";

// LVGL's C callbacks can't carry a `this` pointer, and this board only ever
// has one display, so a single static instance pointer is the simplest way
// to route the callbacks back into the bridge.
LvglBridge *g_bridge = nullptr;
}  // namespace

bool LvglBridge::begin(DisplayDriver &display, TouchDriver &touch) {
  Logger::info(kTag, "begin() start");
  display_ = &display;
  touch_ = &touch;
  g_bridge = this;

  lv_init();
  Logger::info(kTag, "lv_init() done, allocating draw buffers");

  size_t drawBufSize = display_->width() * kDrawBufHeight;
  drawBuf1_ = static_cast<lv_color_t *>(
      heap_caps_malloc(drawBufSize * sizeof(lv_color_t), MALLOC_CAP_DMA));
  drawBuf2_ = static_cast<lv_color_t *>(
      heap_caps_malloc(drawBufSize * sizeof(lv_color_t), MALLOC_CAP_DMA));
  if (!drawBuf1_ || !drawBuf2_) {
    Logger::error(kTag, "draw buffer allocation failed");
    return false;
  }

  lvDisplay_ = lv_display_create(display_->width(), display_->height());
  lv_display_set_flush_cb(lvDisplay_, flushCallback);
  lv_display_set_buffers(lvDisplay_, drawBuf1_, drawBuf2_, drawBufSize,
                          LV_DISPLAY_RENDER_MODE_PARTIAL);

  lvTouchpad_ = lv_indev_create();
  lv_indev_set_type(lvTouchpad_, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(lvTouchpad_, touchReadCallback);

  const esp_timer_create_args_t tickTimerArgs = {
      .callback = &tickTimerCallback,
      .name = "lvgl_tick",
  };
  esp_timer_handle_t tickTimer;
  esp_timer_create(&tickTimerArgs, &tickTimer);
  esp_timer_start_periodic(tickTimer, kLvglTickPeriodMs * 1000);

  Logger::info(kTag, "begin() complete");
  return true;
}

void LvglBridge::tick() { lv_timer_handler(); }

void LvglBridge::flushCallback(lv_display_t *disp, const lv_area_t *area,
                                uint8_t *pixelMap) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  g_bridge->display_->gfx()->draw16bitRGBBitmap(area->x1, area->y1,
                                                 (uint16_t *)pixelMap, w, h);
  lv_display_flush_ready(disp);
}

void LvglBridge::touchReadCallback(lv_indev_t *indev, lv_indev_data_t *data) {
  uint16_t x, y;
  if (g_bridge->touch_->read(x, y)) {
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
    ActivityMonitor::notify();
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void LvglBridge::tickTimerCallback(void *arg) {
  lv_tick_inc(kLvglTickPeriodMs);
}
