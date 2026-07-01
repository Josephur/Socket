#include "OfflineScreen.h"

#include "../fonts/AiIcons.h"

namespace {
lv_obj_t *g_screen = nullptr;

// TEMPORARY test row for the new AiIcons font subset -- see AiIcons.h for
// where these come from (a real FiraCode Nerd Font Mono TTF, verified
// codepoints, converted with lv_font_conv). Put here rather than on
// IdleScreen because this is the screen that's actually visible without a
// real provisioning server -- move to wherever these icons are actually
// used (status bar, conversation screen, etc) once picked.
void addIconTestRow(lv_obj_t *parent) {
  static const char *kIcons[] = {
      AiIcons::kRobot,     AiIcons::kBrain,      AiIcons::kMicrochip,
      AiIcons::kSparkle,   AiIcons::kMagicWand,  AiIcons::kComment,
      AiIcons::kComments,  AiIcons::kMicrophone, AiIcons::kVolumeUp,
      AiIcons::kVolumeOff, AiIcons::kWifi,       AiIcons::kBluetooth,
      AiIcons::kCloud,     AiIcons::kDatabase,   AiIcons::kGear,
      AiIcons::kBolt,      AiIcons::kCheck,      AiIcons::kClose,
      AiIcons::kWarning,   AiIcons::kBatteryFull,
  };

  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, LV_PCT(90), 220);
  lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                         LV_FLEX_ALIGN_CENTER);

  for (const char *icon : kIcons) {
    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_font(label, &lv_font_ai_icons_24, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_pad_all(label, 8, 0);
  }
}
}  // namespace

namespace OfflineScreen {

lv_obj_t *create() {
  g_screen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(g_screen, lv_color_black(), 0);

  lv_obj_t *label = lv_label_create(g_screen);
  lv_label_set_text(label, "Offline -- reconnecting...");
  lv_obj_set_style_text_color(label, lv_color_hex(0xFF5252), 0);
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 60);

  addIconTestRow(g_screen);

  return g_screen;
}

void show() { lv_scr_load(g_screen); }

}  // namespace OfflineScreen
