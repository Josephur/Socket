#include "OfflineScreen.h"

#include "../icons/AiEmoji.h"

namespace {
lv_obj_t *g_screen = nullptr;

// TEMPORARY test row for the AiEmoji full-color icon set -- see AiEmoji.h
// for where these come from (Twemoji SVGs rasterized and converted to LVGL
// image descriptors). Replaced the earlier monochrome AiIcons font test row
// here (same spot) per explicit request -- the mono font code itself is
// still in the repo (src/ui/fonts/AiIcons.h) for later use where a
// single-tint icon fits better, just not shown on screen right now. Put
// here rather than on IdleScreen because this is the screen that's
// actually visible without a real provisioning server -- move to wherever
// these icons are actually used (status bar, conversation screen, etc)
// once picked.
void addIconTestRow(lv_obj_t *parent) {
  static const lv_image_dsc_t *kIcons[] = {
      AiEmoji::kRobot,          AiEmoji::kBrain,        AiEmoji::kSparkles,
      AiEmoji::kSpeechBalloon,  AiEmoji::kThoughtBalloon,
      AiEmoji::kMagicWand,      AiEmoji::kMicrophone,   AiEmoji::kSpeaker,
      AiEmoji::kSpeakerMuted,   AiEmoji::kSignal,       AiEmoji::kCloud,
      AiEmoji::kGear,           AiEmoji::kLightning,    AiEmoji::kCheck,
      AiEmoji::kCross,          AiEmoji::kWarning,      AiEmoji::kBattery,
  };

  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, LV_PCT(90), 220);
  lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                         LV_FLEX_ALIGN_CENTER);

  for (const lv_image_dsc_t *icon : kIcons) {
    lv_obj_t *img = lv_image_create(row);
    lv_image_set_src(img, icon);
    lv_obj_set_style_pad_all(img, 8, 0);
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
