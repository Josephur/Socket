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

// Set to 1 to boot straight into the speaker/mic bring-up test instead of
// the normal app: runs an automated tone-detection self-test, then a live
// mic->speaker loopback forever (talk into the mic, listen on the speaker).
// Skips WiFi/display/LVGL entirely so this is a pure audio-path test with
// no other subsystems in the way. Set back to 0 to return to normal.
#define SOCKET_AUDIO_TEST_MODE 1

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

// Attempts WiFi connect + provisioning fetch. Transitions to kIdle on
// success or kOffline on failure (see AppState.h for the OFFLINE
// recovery path: it always re-enters kProvisioning).
void runProvisioning() {
  Logger::info(kTag, "runProvisioning() start");
  if (!g_wifi.isConnected() && !g_wifi.begin()) {
    Logger::error(kTag, "WiFi connect failed, seeding fallback credentials");
    if (!g_wifi.setCredentials(SOCKET_WIFI_SSID, SOCKET_WIFI_PASSWORD)) {
      g_appState.transitionTo(AppState::kOffline);
      return;
    }
  }

  if (!ProvisioningClient::fetch(SOCKET_PROVISIONING_URL, g_config)) {
    Logger::error(kTag, "Provisioning fetch failed");
    g_appState.transitionTo(AppState::kOffline);
    return;
  }

  Logger::info(kTag, ("Provisioned as: " + g_config.deviceName).c_str());
  g_provisioningBackoff.reset();
  g_appState.transitionTo(AppState::kIdle);
  IdleScreen::show();
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

  Logger::info(kTag, "step: audioCodec.begin()");
  if (!g_audioCodec.begin()) {
    Logger::warn(kTag, "AudioCodec not fully initialized (see AudioCodec.h)");
  }

  Logger::info(
      kTag,
      ("free DMA heap before WiFi: " +
       String(heap_caps_get_free_size(MALLOC_CAP_DMA)) + " bytes")
          .c_str());

  Logger::info(kTag, "step: runProvisioning()");
  runProvisioning();
  if (g_appState.current() == AppState::kOffline) {
    OfflineScreen::show();
  }

  pinMode(kBootButtonPin, INPUT_PULLUP);
  ActivityMonitor::notify();  // start the screen-timeout countdown fresh

  Logger::info(kTag, "=== setup() complete ===");
}

void loop() {
  g_lvgl.tick();
  pollBootButton();
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

    case AppState::kOffline:
      // Gated by the backoff schedule so a down WiFi network or an
      // unreachable provisioning server doesn't get hammered every 5ms --
      // see RetryBackoff.h for the exact schedule.
      if (g_provisioningBackoff.dueNow()) {
        Logger::info(kTag, "OFFLINE retry due -- attempting reconnect");
        if (g_wifi.reconnect()) {
          runProvisioning();
          if (g_appState.current() == AppState::kOffline) {
            OfflineScreen::show();
          }
        }
      }
      break;

    default:
      // kListening/kThinking/kSpeaking are driven synchronously inside
      // runConversationTurn(); kBoot/kProvisioning only happen in setup().
      break;
  }

  delay(5);
}

#endif  // SOCKET_AUDIO_TEST_MODE
