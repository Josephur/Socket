#include "IdleScreen.h"

namespace {
lv_obj_t *g_screen = nullptr;
lv_obj_t *g_clockLabel = nullptr;
}  // namespace

namespace IdleScreen {

lv_obj_t *create() {
  g_screen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(g_screen, lv_color_black(), 0);

  g_clockLabel = lv_label_create(g_screen);
  lv_label_set_text(g_clockLabel, "--:--");
  lv_obj_set_style_text_color(g_clockLabel, lv_color_white(), 0);
  lv_obj_align(g_clockLabel, LV_ALIGN_TOP_MID, 0, 40);

  lv_obj_t *hint = lv_label_create(g_screen);
  lv_label_set_text(hint, "Say the wake word to talk");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
  lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);

  // Reserved for future shop ticket/appointment integration -- the API for
  // this doesn't exist yet (see the plan), but the space is carved out now
  // so that feature is a drop-in later rather than a UI redesign.
  lv_obj_t *shopIntegrationSlot = lv_obj_create(g_screen);
  lv_obj_set_size(shopIntegrationSlot, LV_PCT(90), 120);
  lv_obj_align(shopIntegrationSlot, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_set_style_bg_opa(shopIntegrationSlot, LV_OPA_10, 0);
  lv_obj_set_style_border_color(shopIntegrationSlot, lv_color_hex(0x444444), 0);

  return g_screen;
}

void show() { lv_scr_load(g_screen); }

void updateClock(const String &timeText) {
  if (g_clockLabel) lv_label_set_text(g_clockLabel, timeText.c_str());
}

}  // namespace IdleScreen
