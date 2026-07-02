#pragma once

#include "../audio/AudioCodec.h"

// TEMPORARY dev tooling for audio bring-up (like the touch-indicator dot):
// a speaker-icon button and a volume slider floating on lv_layer_top(), so
// they're usable above whatever screen is showing.
//
//  - Button press: on a background task, beep -> record 3s -> beep -> play
//    the recording back. The button dims while a run is in flight and
//    ignores re-taps.
//  - Slider: writes the ES8311 DAC volume (0-100%) immediately, including
//    mid-playback, with a live % label.
//
// Remove once real conversation audio is wired up end to end.
namespace AudioTestPanel {

// Builds the widgets on lv_layer_top(). Call once after LVGL init, before
// loop() starts. Keeps a reference to `codec` for the demo task + slider.
void create(AudioCodec &codec);

// Call every loop() tick: restores the button's idle look once a demo task
// finishes (LVGL isn't thread-safe, so the task can't do it itself).
void tick();

}  // namespace AudioTestPanel
