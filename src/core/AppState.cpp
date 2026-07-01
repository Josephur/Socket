#include "AppState.h"

#include "ActivityMonitor.h"

const char *toString(AppState state) {
  switch (state) {
    case AppState::kBoot:
      return "BOOT";
    case AppState::kProvisioning:
      return "PROVISIONING";
    case AppState::kIdle:
      return "IDLE";
    case AppState::kListening:
      return "LISTENING";
    case AppState::kThinking:
      return "THINKING";
    case AppState::kSpeaking:
      return "SPEAKING";
    case AppState::kOffline:
      return "OFFLINE";
  }
  return "UNKNOWN";
}

bool AppStateMachine::transitionTo(AppState next) {
  // OFFLINE is reachable from anywhere (WiFi/C6 link or a server request can
  // fail mid-conversation) and recovery always re-provisions, since v1 only
  // fetches config once and the device may have missed updates while down.
  if (next == AppState::kOffline) {
    state_ = next;
    ActivityMonitor::notify();
    return true;
  }

  bool valid = false;
  switch (state_) {
    case AppState::kBoot:
      valid = (next == AppState::kProvisioning);
      break;
    case AppState::kProvisioning:
      valid = (next == AppState::kIdle);
      break;
    case AppState::kIdle:
      valid = (next == AppState::kListening);
      break;
    case AppState::kListening:
      valid = (next == AppState::kThinking || next == AppState::kIdle);
      break;
    case AppState::kThinking:
      valid = (next == AppState::kSpeaking || next == AppState::kIdle);
      break;
    case AppState::kSpeaking:
      valid = (next == AppState::kIdle);
      break;
    case AppState::kOffline:
      valid = (next == AppState::kProvisioning);
      break;
  }

  if (valid) {
    state_ = next;
    ActivityMonitor::notify();
  }
  return valid;
}
