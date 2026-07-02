// Socket -- AI assistant firmware for the Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
// See docs/PINOUT.md, docs/SETUP.md, and the project plan for background.
//
// This file only wires modules together and drives the AppState state
// machine; the actual logic lives under src/. See README.md for the
// directory layout.

#if __has_include("src/config/Secrets.h")
#include "src/config/Secrets.h"
#else
#error \
    "src/config/Secrets.h is missing. Copy src/config/Secrets.h.example to " \
    "src/config/Secrets.h and fill in your WiFi credentials + provisioning URL."
#endif

#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// Set to 1 to boot straight into the speaker/mic bring-up test instead of
// the normal app: runs an automated tone-detection self-test, then a live
// mic->speaker loopback forever (talk into the mic, listen on the speaker).
// Skips WiFi/display/LVGL entirely so this is a pure audio-path test with
// no other subsystems in the way -- useful for isolating audio issues, but
// it means no video signal at all (screen goes black, though the backlight
// GPIO floats "on" by default since nothing drives it either way).
//
// Normal mode (0) now also runs the tone self-test once during boot (see
// setup() below) so you get audio diagnostics AND the full screen/touch/
// timeout behavior in the same build.
#define SOCKET_AUDIO_TEST_MODE 0

#include "src/audio/AudioCodec.h"
#include "src/audio/AudioPlayer.h"
#include "src/audio/AudioRecorder.h"
#include "src/audio/AudioSelfTest.h"
#include "src/audio/WakeWordDetector.h"
#include "src/config/DeviceIdentity.h"
#include "src/config/ProvisioningClient.h"
#include "src/config/RuntimeConfig.h"
#include "src/core/ActivityMonitor.h"
#include "src/core/AppState.h"
#include "src/core/Logger.h"
#include "src/core/RetryBackoff.h"
#include "src/network/ConversationClient.h"
#include "src/network/WiFiManager.h"
#include "src/ui/DisplayDriver.h"
#include "src/ui/LvglBridge.h"
#include "src/ui/TouchDriver.h"
#include "src/ui/screens/ConversationScreen.h"
#include "src/ui/screens/IdleScreen.h"
#include "src/ui/screens/OfflineScreen.h"

namespace {
constexpr const char *kTag = "Socket";

// BOOT button, GPIO35 per docs/PINOUT.md -- active low. Doubles as a wake
// button for the screen-timeout feature below.
constexpr int kBootButtonPin = 35;

// Screen blanks (backlight off) after this long with no activity. "Activity"
// is centrally defined by ActivityMonitor -- see its header for the current
// list of sources (state transitions, touch, this button).
constexpr uint32_t kScreenTimeoutMs = 5UL * 60UL * 1000UL;

AppStateMachine g_appState;
RuntimeConfig g_config;

DisplayDriver g_display;
TouchDriver g_touch;
LvglBridge g_lvgl;

WiFiManager g_wifi;
// Gates OFFLINE-state retries (WiFi reconnect + provisioning fetch) so a
// down/unreachable server gets hit on a 3,5,10,15,30,60s-then-60s schedule
// instead of every loop() tick.
RetryBackoff g_provisioningBackoff;

AudioCodec g_audioCodec;
WakeWordDetector g_wakeWord(g_audioCodec);
AudioRecorder g_recorder(g_audioCodec);
AudioPlayer g_player(g_audioCodec);

// WiFi connect + provisioning fetch both make blocking network calls (WiFi
// association, then an HTTPS GET) that can take up to the better part of a
// minute when the AP or the provisioning server is slow/unreachable. Running
// that directly from loop() -- as this used to -- froze loop() for the
// duration, which meant touch input and the audio self-test (both polled
// from loop(), see pollTouchForTestTone() below) went dead every time the
// OFFLINE retry fired. Since touch/audio have nothing to do with the network
// stack, they should never be held hostage by it.
//
// Fix: run the network attempt on its own FreeRTOS task, pinned to core 0
// (alongside the WiFi/BLE stack) so it never contends with the core-1 loop()
// task for CPU time. loop() just polls a queue for the result -- it never
// blocks waiting for the task.
struct NetworkAttemptResult {
  bool success;
  RuntimeConfig config;
};

// The queue carries a heap-allocated pointer, not the struct by value:
// xQueueSend() copies items with a raw memcpy, which would bypass String's
// copy constructor and corrupt RuntimeConfig's String fields (deviceName
// etc.) if NetworkAttemptResult itself were the queue item type. A pointer
// is trivially copyable, so memcpy-ing it is safe; loop() takes ownership
// and deletes it after use (see applyNetworkAttemptResult()'s caller).
QueueHandle_t g_networkResultQueue = nullptr;
volatile bool g_networkAttemptInFlight = false;

void networkAttemptTask(void * /*unused*/) {
  auto *result = new NetworkAttemptResult();
  result->success = false;

  Logger::info(kTag, "networkAttemptTask() start");
  if (!g_wifi.isConnected() && !g_wifi.reconnect()) {
    Logger::error(kTag, "WiFi connect failed, seeding fallback credentials");
    if (!g_wifi.setCredentials(SOCKET_WIFI_SSID, SOCKET_WIFI_PASSWORD)) {
      xQueueSend(g_networkResultQueue, &result, 0);
      g_networkAttemptInFlight = false;
      vTaskDelete(nullptr);
      return;
    }
  }

  if (!ProvisioningClient::fetch(SOCKET_PROVISIONING_URL, result->config)) {
    Logger::error(kTag, "Provisioning fetch failed");
    xQueueSend(g_networkResultQueue, &result, 0);
    g_networkAttemptInFlight = false;
    vTaskDelete(nullptr);
    return;
  }

  Logger::info(kTag, ("Provisioned as: " + result->config.deviceName).c_str());
  result->success = true;
  xQueueSend(g_networkResultQueue, &result, 0);
  g_networkAttemptInFlight = false;
  vTaskDelete(nullptr);
}

// Kicks off networkAttemptTask() if one isn't already running. Safe to call
// every loop() tick -- it's a no-op while an attempt is in flight.
void startNetworkAttemptIfIdle() {
  if (g_networkAttemptInFlight) return;
  g_networkAttemptInFlight = true;
  xTaskCreatePinnedToCore(networkAttemptTask, "net-attempt", 8192, nullptr,
                           tskIDLE_PRIORITY + 1, nullptr, /*core=*/0);
}

// Applies a finished networkAttemptTask() result on the main thread (never
// from the task itself -- AppState and the LVGL-backed screens aren't
// thread-safe). Called from loop() once the result queue has something.
void applyNetworkAttemptResult(const NetworkAttemptResult &result) {
  if (result.success) {
    g_config = result.config;
    g_provisioningBackoff.reset();
    g_appState.transitionTo(AppState::kIdle);
    IdleScreen::show();
  } else {
    g_appState.transitionTo(AppState::kOffline);
    OfflineScreen::show();
  }
}

void runConversationTurn() {
  Logger::info(kTag, "runConversationTurn() start");
  g_appState.transitionTo(AppState::kListening);
  ConversationScreen::show();
  ConversationScreen::setState(AppState::kListening);

  size_t sampleCount = 0;
  int16_t *utterance = g_recorder.recordUtterance(sampleCount);
  if (!utterance || sampleCount == 0) {
    Logger::error(kTag, "Recording failed");
    g_appState.transitionTo(AppState::kIdle);
    IdleScreen::show();
    return;
  }

  g_appState.transitionTo(AppState::kThinking);
  ConversationScreen::setState(AppState::kThinking);

  ConversationResult result = ConversationClient::converse(
      g_config.converseEndpoint, reinterpret_cast<uint8_t *>(utterance),
      sampleCount * sizeof(int16_t));
  free(utterance);

  if (!result.ok) {
    Logger::error(kTag, "Conversation turn failed");
    g_appState.transitionTo(AppState::kOffline);
    return;
  }

  ConversationScreen::setTranscript(result.transcript);
  g_appState.transitionTo(AppState::kSpeaking);
  ConversationScreen::setState(AppState::kSpeaking);
  g_player.play(result.audioData, result.audioLen, result.audioFormat);
  free(result.audioData);

  g_appState.transitionTo(AppState::kIdle);
  IdleScreen::show();
}

// Blanks the backlight after kScreenTimeoutMs of no ActivityMonitor
// activity, and restores it the moment activity resumes. Rendering itself
// keeps running underneath -- this only cuts the backlight, per the current
// requirement ("go blank and 0 backlight until activity resumes").
void updateScreenPower() {
  static bool screenOn = true;
  bool shouldBeOn = ActivityMonitor::msSinceLastActivity() < kScreenTimeoutMs;

  if (shouldBeOn != screenOn) {
    screenOn = shouldBeOn;
    g_display.setBacklight(screenOn);
    Logger::info(kTag, screenOn ? "screen woke (activity detected)"
                                 : "screen timed out after 5 min idle");
  }
}

// Polls the BOOT button as a physical wake source. Debounced with a simple
// edge check so holding it doesn't spam ActivityMonitor -- harmless either
// way, but keeps the log quiet.
void pollBootButton() {
  static bool wasPressed = false;
  bool pressed = digitalRead(kBootButtonPin) == LOW;
  if (pressed && !wasPressed) {
    Logger::info(kTag, "BOOT button pressed");
    ActivityMonitor::notify();
  }
  wasPressed = pressed;
}

// TEMPORARY, for speaker/mic bring-up testing: run the record/playback demo
// (3 beeps, record, 3 beeps, play it back) on every fresh touch tap,
// independent of whatever's on screen. Polls the touch controller directly
// (separate from LVGL's own indev polling) so this works regardless of what
// widget, if any, is under the touch point.
//
// The demo itself takes ~5 seconds, so it runs on its own task instead of
// inline in loop() -- otherwise it would freeze touch/LVGL/BOOT-button
// polling for its whole duration, same problem as the old blocking WiFi
// calls above. g_audioDemoInFlight guards against overlapping runs (a tap
// mid-demo is just ignored) since AudioCodec isn't meant to be driven from
// two tasks at once.
//
// Remove once audio bring-up is confirmed working and this is no longer
// needed as a quick manual trigger.
volatile bool g_audioDemoInFlight = false;

void audioDemoTask(void * /*unused*/) {
  AudioSelfTest::runRecordPlaybackDemo(g_audioCodec);
  g_audioDemoInFlight = false;
  vTaskDelete(nullptr);
}

// TEMPORARY debug aid: a small dot that follows the touch point on top of
// whatever screen is showing, so a press can be visually confirmed even if
// downstream behavior (audio, screen transitions) isn't working -- answers
// "is touch registering at all?" independent of everything else. Lives on
// lv_layer_top() so it draws above every screen without needing to be
// added to each one individually, and is non-clickable so it never steals
// touch input from the real UI underneath.
//
// Remove once touch + audio are both confirmed working end to end.
lv_obj_t *g_touchIndicator = nullptr;

void createTouchIndicator() {
  g_touchIndicator = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(g_touchIndicator);
  lv_obj_set_size(g_touchIndicator, 28, 28);
  lv_obj_set_style_radius(g_touchIndicator, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(g_touchIndicator, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_bg_opa(g_touchIndicator, LV_OPA_70, 0);
  lv_obj_clear_flag(g_touchIndicator, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(g_touchIndicator, LV_OBJ_FLAG_HIDDEN);
}

// Single touch poll per loop() tick, shared by the audio-demo trigger and
// the indicator dot -- previously each had its own g_touch.read() call,
// doubling I2C traffic on the shared bus for no reason. Also logs every
// raw press/release edge with a timestamp (temporary -- see the "taps do
// nothing" debugging this is for) so we can tell whether the touch
// controller itself is missing quick taps versus this loop just not
// polling often enough to catch them.
void pollTouch() {
  static bool wasPressed = false;
  uint16_t x, y;
  bool pressed = g_touch.read(x, y);

  if (pressed != wasPressed) {
    Logger::info(kTag, (String("touch edge: ") + (pressed ? "DOWN" : "UP") +
                         " at t=" + String(millis()) +
                         (pressed ? (" (" + String(x) + "," + String(y) + ")")
                                  : ""))
                            .c_str());
  }

  if (pressed) {
    lv_obj_set_pos(g_touchIndicator, x - 14, y - 14);
    lv_obj_clear_flag(g_touchIndicator, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(g_touchIndicator, LV_OBJ_FLAG_HIDDEN);
  }

  if (pressed && !wasPressed && !g_audioDemoInFlight) {
    Logger::info(kTag, "Touch tap detected -- running record/playback demo");
    g_audioDemoInFlight = true;
    xTaskCreatePinnedToCore(audioDemoTask, "audio-demo", 8192, nullptr,
                             tskIDLE_PRIORITY + 1, nullptr, /*core=*/1);
  }
  wasPressed = pressed;
}

}  // namespace

#if SOCKET_AUDIO_TEST_MODE
void setup() {
  Logger::begin();
  delay(1500);
  Logger::info(kTag, "=== Socket AUDIO TEST MODE ===");
  Logger::info(kTag, "WiFi/display/LVGL skipped -- pure audio-path test");

  if (!g_audioCodec.begin()) {
    Logger::error(kTag,
                   "AudioCodec.begin() failed -- see Es8311/Es7210 logs "
                   "above for which chip didn't ACK on I2C");
    while (true) {
      delay(1000);
    }
  }

  AudioSelfTest::ToneTestResult result =
      AudioSelfTest::runToneDetectionTest(g_audioCodec);
  Logger::info(kTag, result.toneDetected
                          ? "SELF-TEST PASSED: mic picked up the played "
                            "tone -- speaker and mic paths are both alive"
                          : "SELF-TEST INCONCLUSIVE: mic did not clearly "
                            "pick up the tone -- do the manual listening "
                            "test next to tell physical vs. register issue");

  AudioSelfTest::runLiveLoopbackForever(g_audioCodec);  // never returns
}

void loop() {}  // unreachable in audio test mode

#else

void setup() {
  Logger::begin();
  // Give the USB-serial bridge time to enumerate before we start printing --
  // without this, early boot log lines can be lost even though the chip
  // itself is running fine. Matches the delay used in the proven
  // docs/Arduino/examples/LVGLV9_Arduino reference sketch.
  delay(1500);

  Logger::info(kTag, "=== Socket booting ===");
  Logger::info(kTag, ("chip id " + DeviceIdentity::chipId()).c_str());
  g_appState.transitionTo(AppState::kProvisioning);

  Logger::info(kTag, "step: display.begin()");
  if (!g_display.begin()) {
    Logger::error(kTag, "Display init failed");
  }

  Logger::info(kTag, "step: touch.begin()");
  if (!g_touch.begin()) {
    Logger::error(kTag, "Touch init failed");
  }

  Logger::info(kTag, "step: lvgl.begin()");
  if (!g_lvgl.begin(g_display, g_touch)) {
    Logger::error(kTag, "LVGL init failed");
  }

  Logger::info(kTag, "step: creating UI screens");
  IdleScreen::create();
  ConversationScreen::create();
  OfflineScreen::create();
  createTouchIndicator();

  Logger::info(kTag, "step: audioCodec.begin()");
  if (!g_audioCodec.begin()) {
    Logger::warn(kTag, "AudioCodec not fully initialized (see AudioCodec.h)");
  } else {
    Logger::info(kTag, "running audio self-test (tone + record)...");
    AudioSelfTest::ToneTestResult audioResult =
        AudioSelfTest::runToneDetectionTest(g_audioCodec);
    Logger::info(kTag, audioResult.toneDetected
                            ? "audio SELF-TEST PASSED"
                            : "audio SELF-TEST INCONCLUSIVE -- see "
                              "AudioSelfTest logs above");
  }

  Logger::info(
      kTag,
      ("free DMA heap before WiFi: " +
       String(heap_caps_get_free_size(MALLOC_CAP_DMA)) + " bytes")
          .c_str());

  // Deliberately NOT attempting a network connection here: loop() -- and
  // therefore all touch/button polling -- doesn't start until setup()
  // returns, so doing it here would make the screen look fully booted
  // (display was already up) while inputs were silently ignored for as long
  // as it took. Instead, go straight to OFFLINE and let loop()'s existing
  // RetryBackoff-gated retry (below) kick off the first background network
  // attempt a few seconds in, once loop() is already running and
  // responsive.
  Logger::info(kTag, "deferring first provisioning attempt to loop()");
  g_appState.transitionTo(AppState::kOffline);
  OfflineScreen::show();

  pinMode(kBootButtonPin, INPUT_PULLUP);
  ActivityMonitor::notify();  // start the screen-timeout countdown fresh

  g_networkResultQueue = xQueueCreate(1, sizeof(NetworkAttemptResult *));

  Logger::info(kTag, "=== setup() complete ===");
}

void loop() {
  // TEMPORARY diagnostic for "taps do nothing" -- if a single loop() tick
  // takes an unusually long time, touch polling stalls for exactly that
  // long and quick taps get missed even though the touch controller itself
  // saw them. 20ms is well above the 5ms delay() at the bottom of this
  // loop, so anything over that means something in the tick (LVGL render,
  // I2C contention, etc) is the real bottleneck, not the polling code.
  static uint32_t lastLoopMs = 0;
  uint32_t nowMs = millis();
  uint32_t loopDeltaMs = nowMs - lastLoopMs;
  lastLoopMs = nowMs;
  if (loopDeltaMs > 20) {
    Logger::warn(kTag, (String("slow loop() tick: ") + String(loopDeltaMs) +
                         "ms at t=" + String(nowMs))
                            .c_str());
  }

  g_lvgl.tick();
  pollBootButton();
  pollTouch();
  updateScreenPower();

  // Throttled heartbeat so a hung/silent board is distinguishable from one
  // that's simply idle -- see the current AppState if this stops printing.
  static uint32_t lastHeartbeatMs = 0;
  if (millis() - lastHeartbeatMs > 5000) {
    lastHeartbeatMs = millis();
    Logger::info(kTag, (String("heartbeat, state=") + toString(g_appState.current())).c_str());
  }

  switch (g_appState.current()) {
    case AppState::kIdle:
      if (g_wakeWord.checkForWakeWord()) {
        runConversationTurn();
      }
      break;

    case AppState::kOffline: {
      // Gated by the backoff schedule so a down WiFi network or an
      // unreachable provisioning server doesn't get hammered every 5ms --
      // see RetryBackoff.h for the exact schedule. The actual attempt runs
      // on a background task (see startNetworkAttemptIfIdle() above) so
      // this never blocks loop().
      if (g_provisioningBackoff.dueNow()) {
        Logger::info(kTag, "OFFLINE retry due -- starting network attempt");
        startNetworkAttemptIfIdle();
      }
      NetworkAttemptResult *result = nullptr;
      if (xQueueReceive(g_networkResultQueue, &result, 0) == pdTRUE) {
        applyNetworkAttemptResult(*result);
        delete result;
      }
      break;
    }

    default:
      // kListening/kThinking/kSpeaking are driven synchronously inside
      // runConversationTurn(); kBoot/kProvisioning only happen in setup().
      break;
  }

  delay(5);
}

#endif  // SOCKET_AUDIO_TEST_MODE
