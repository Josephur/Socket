#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "../../core/AppState.h"

// Shown for kListening/kThinking/kSpeaking: an avatar whose color reflects
// which of those three it is, plus a live transcript label.
namespace ConversationScreen {

lv_obj_t *create();
void show();

// Recolors the avatar for the given state (only kListening/kThinking/
// kSpeaking are meaningful here).
void setState(AppState state);

void setTranscript(const String &text);

}  // namespace ConversationScreen
