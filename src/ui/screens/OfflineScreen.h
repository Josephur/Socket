#pragma once

#include <lvgl.h>

// Shown for AppState::kOffline. Per the plan's "Offline behavior" decision:
// a clear status indicator only, no assistant functionality while
// disconnected.
namespace OfflineScreen {

lv_obj_t *create();
void show();

}  // namespace OfflineScreen
