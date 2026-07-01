#pragma once

#include <Arduino.h>
#include <lvgl.h>

// Home/idle screen: shown whenever AppState is kIdle. Has a clock area and a
// visibly reserved region for the future shop ticket/appointment
// integration (API not designed yet -- see the plan's "Deferred / roadmap").
namespace IdleScreen {

// Builds the screen once; call show() on subsequent AppState transitions.
lv_obj_t *create();
void show();

// Call periodically (e.g. once a second) to refresh the clock label.
void updateClock(const String &timeText);

}  // namespace IdleScreen
