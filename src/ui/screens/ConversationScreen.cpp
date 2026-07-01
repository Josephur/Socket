#include "ConversationScreen.h"

namespace {
lv_obj_t *g_screen = nullptr;
lv_obj_t *g_avatar = nullptr;
lv_obj_t *g_transcriptLabel = nullptr;
}  // namespace

namespace ConversationScreen {

lv_obj_t *create() {
  g_screen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(g_screen, lv_color_black(), 0);

  g_avatar = lv_obj_create(g_screen);
  lv_obj_set_size(g_avatar, 160, 160);
  lv_obj_set_style_radius(g_avatar, LV_RADIUS_CIRCLE, 0);
  lv_obj_align(g_avatar, LV_ALIGN_CENTER, 0, -80);

  g_transcriptLabel = lv_label_create(g_screen);
  lv_label_set_long_mode(g_transcriptLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(g_transcriptLabel, LV_PCT(85));
  lv_obj_set_style_text_align(g_transcriptLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(g_transcriptLabel, lv_color_white(), 0);
  lv_label_set_text(g_transcriptLabel, "");
  lv_obj_align(g_transcriptLabel, LV_ALIGN_BOTTOM_MID, 0, -40);

  return g_screen;
}

void show() { lv_scr_load(g_screen); }

void setState(AppState state) {
  if (!g_avatar) return;
  uint32_t color;
  switch (state) {
    case AppState::kListening:
      color = 0x2196F3;  // Blue: actively recording.
      break;
    case AppState::kThinking:
      color = 0xFFC107;  // Amber: waiting on the server.
      break;
    case AppState::kSpeaking:
      color = 0x4CAF50;  // Green: playing the reply.
      break;
    default:
      color = 0x555555;
      break;
  }
  lv_obj_set_style_bg_color(g_avatar, lv_color_hex(color), 0);
}

void setTranscript(const String &text) {
  if (g_transcriptLabel) lv_label_set_text(g_transcriptLabel, text.c_str());
}

}  // namespace ConversationScreen
