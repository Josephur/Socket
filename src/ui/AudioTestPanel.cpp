#include "AudioTestPanel.h"

#include <lvgl.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../audio/AudioSelfTest.h"
#include "../core/ActivityMonitor.h"
#include "../core/Logger.h"
#include "icons/AiEmoji.h"

namespace {
constexpr const char *kTag = "AudioTestPanel";
// Keep in sync with the boot default in Es8311::begin(). ~+16.5dB with the
// -30..+32dB slider mapping in Es8311::setVolume() -- picked by ear during
// bring-up ("about 75% should be the default").
constexpr uint8_t kDefaultVolumePercent = 75;

AudioCodec *g_codec = nullptr;
lv_obj_t *g_button = nullptr;
lv_obj_t *g_volumeLabel = nullptr;

// Guards against overlapping demo runs (AudioCodec isn't meant to be driven
// from two tasks at once). Set on the LVGL/main task, cleared by the demo
// task; tick() polls it to restore the button's idle look.
volatile bool g_demoInFlight = false;
bool g_buttonDimmed = false;

void demoTask(void * /*unused*/) {
  AudioSelfTest::runRecordPlaybackDemo(*g_codec);
  g_demoInFlight = false;
  vTaskDelete(nullptr);
}

void onButtonClicked(lv_event_t * /*event*/) {
  ActivityMonitor::notify();
  if (g_demoInFlight) {
    Logger::info(kTag, "demo already running, tap ignored");
    return;
  }
  Logger::info(kTag, "speaker button pressed -- beep, record 3s, playback");
  g_demoInFlight = true;
  g_buttonDimmed = true;
  lv_obj_set_style_opa(g_button, LV_OPA_30, 0);
  xTaskCreatePinnedToCore(demoTask, "audio-demo", 8192, nullptr,
                           tskIDLE_PRIORITY + 1, nullptr, /*core=*/1);
}

void onVolumeChanged(lv_event_t *event) {
  ActivityMonitor::notify();
  auto *slider = static_cast<lv_obj_t *>(lv_event_get_target(event));
  int32_t volume = lv_slider_get_value(slider);
  g_codec->setVolume(static_cast<uint8_t>(volume));
  lv_label_set_text_fmt(g_volumeLabel, "%d%%", (int)volume);
}

void beepTask(void * /*unused*/) {
  AudioSelfTest::playBeeps(*g_codec, 1);
  g_demoInFlight = false;
  vTaskDelete(nullptr);
}

// Releasing the slider plays one beep at the just-set volume, so a sweep
// can be auditioned level by level without reaching for the demo button.
// Reuses the demo's in-flight guard: skip the beep rather than drive the
// codec from two tasks (and a demo's own beeps already give feedback).
void onVolumeReleased(lv_event_t * /*event*/) {
  ActivityMonitor::notify();
  if (g_demoInFlight) return;
  g_demoInFlight = true;
  xTaskCreatePinnedToCore(beepTask, "vol-beep", 4096, nullptr,
                           tskIDLE_PRIORITY + 1, nullptr, /*core=*/1);
}
}  // namespace

namespace AudioTestPanel {

void create(AudioCodec &codec) {
  g_codec = &codec;
  lv_obj_t *layer = lv_layer_top();

  g_button = lv_button_create(layer);
  lv_obj_set_size(g_button, 56, 56);
  lv_obj_align(g_button, LV_ALIGN_BOTTOM_LEFT, 12, -12);
  lv_obj_add_event_cb(g_button, onButtonClicked, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *icon = lv_image_create(g_button);
  lv_image_set_src(icon, AiEmoji::kSpeaker);
  lv_obj_center(icon);

  lv_obj_t *slider = lv_slider_create(layer);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, kDefaultVolumePercent, LV_ANIM_OFF);
  lv_obj_set_size(slider, LV_PCT(50), 12);
  lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 20, -32);
  lv_obj_add_event_cb(slider, onVolumeChanged, LV_EVENT_VALUE_CHANGED,
                      nullptr);
  lv_obj_add_event_cb(slider, onVolumeReleased, LV_EVENT_RELEASED, nullptr);

  g_volumeLabel = lv_label_create(layer);
  lv_label_set_text_fmt(g_volumeLabel, "%d%%", kDefaultVolumePercent);
  lv_obj_set_style_text_color(g_volumeLabel, lv_color_white(), 0);
  lv_obj_align_to(g_volumeLabel, slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  // The slider's starting position must match what the codec is actually
  // set to -- push our default down rather than trusting the two to agree.
  codec.setVolume(kDefaultVolumePercent);
}

void tick() {
  if (g_buttonDimmed && !g_demoInFlight) {
    g_buttonDimmed = false;
    lv_obj_set_style_opa(g_button, LV_OPA_COVER, 0);
  }
}

}  // namespace AudioTestPanel
