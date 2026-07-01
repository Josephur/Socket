#pragma once

// Central runtime state machine. Both the network layer and the UI screens
// read/react to this state instead of managing their own parallel notion of
// "what's happening right now" -- see the plan's Architecture Overview.

enum class AppState {
  kBoot,          // Powering on, before WiFi/provisioning has been attempted.
  kProvisioning,  // Connecting WiFi + fetching boot config from the PHP server.
  kIdle,          // Config loaded, WiFi up, listening for the wake word.
  kListening,     // Wake word fired, recording the utterance.
  kThinking,      // Utterance sent, awaiting the /api/converse response.
  kSpeaking,      // Playing back the synthesized TTS audio.
  kOffline,       // WiFi/C6 link down or a request to the server failed.
};

const char *toString(AppState state);

class AppStateMachine {
 public:
  AppState current() const { return state_; }

  // Applies the transition and returns true, or returns false and leaves
  // state unchanged if the transition isn't valid from the current state.
  bool transitionTo(AppState next);

 private:
  AppState state_ = AppState::kBoot;
};
