#include "OfflineScreen.h"

namespace {
lv_obj_t *g_screen = nullptr;
}  // namespace

namespace OfflineScreen {

lv_obj_t *create() {
  g_screen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(g_screen, lv_color_black(), 0);

  lv_obj_t *label = lv_label_create(g_screen);
  lv_label_set_text(label, "Offline -- reconnecting...");
  lv_obj_set_style_text_color(label, lv_color_hex(0xFF5252), 0);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  return g_screen;
}

void show() { lv_scr_load(g_screen); }

}  // namespace OfflineScreen
