#pragma once

#include <cstdint>

// Tracks the most recent "activity" timestamp so the UI can blank the
// screen after a period of inactivity. Call notify() from anywhere that
// should count as activity.
//
// Currently wired up to: any successful AppState transition (see
// AppState.cpp -- this covers connectivity changes like OFFLINE<->IDLE, and
// will automatically cover wake-word detection once that transitions into
// kListening), touch input (see LvglBridge's touch callback), and the BOOT
// button (see Socket.ino's loop()). Add more notify() call sites here as
// new activity sources are added -- this is the single place that answers
// "what counts as activity."
namespace ActivityMonitor {

void notify();
uint32_t msSinceLastActivity();

}  // namespace ActivityMonitor
