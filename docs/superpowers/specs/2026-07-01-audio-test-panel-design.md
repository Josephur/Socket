# Audio Test Panel — Design

Date: 2026-07-01
Status: approved (user: "sounds good lets test it all")

## Problem

Audio "stopped working": the speaker is inaudible even though the boot
self-test passes. Root cause of the inaudibility: the ES8311 DAC volume
register is in dB (0x00 = -95.5dB, +0.5dB/step), so the "percent" scale is
wildly nonlinear in loudness — the current 50% default writes -32dB
(whisper-quiet); the mic (gain maxed, next to the speaker) still detects it,
so the self-test keeps "passing". Separately, the previous tap-anywhere
record/playback demo trigger silently depended on touch polling, which was
broken until today's GT911 fix — making it look like audio itself had died.

## Design

A temporary dev panel floating on `lv_layer_top()` (same approach as the
green touch-indicator dot): visible above every screen, no per-screen
wiring, easy to delete when bring-up is done.

Components:

1. **Speaker icon button** (`AiEmoji::kSpeaker`, bottom-left). Press →
   background FreeRTOS task runs: 1 beep → record 3s → 1 beep → play back
   the recording. Button dims while a run is in flight; re-taps ignored.
   (Was: 3 beeps / 3 beeps, triggered by any touch anywhere — that trigger
   is removed.)
2. **Volume slider** (0–100, live % label, bottom edge). Writes the ES8311
   DAC volume register on change; draggable during playback for instant
   feedback.
3. **Default volume: 70%** (≈ -6.5dB, the last audible setting).

## Components / files

- `src/ui/AudioTestPanel.{h,cpp}` — new; owns the button, slider, label,
  the demo task and its in-flight guard. `create(AudioCodec&)` +
  `tick()` (called from loop() to restore button state after the task
  finishes, since LVGL calls must stay on the main task).
- `src/audio/AudioSelfTest.cpp` — `runRecordPlaybackDemo()` beep counts
  change 3→1.
- `src/audio/codecs/Es8311.cpp` — default volume 50→70; comment documents
  the dB mapping so the percent scale stops surprising people.
- `Socket.ino` — remove tap-demo trigger + task from `pollTouch()`; call
  `AudioTestPanel::create()/tick()`.

## Error handling

- Press while running: ignored (volatile in-flight flag).
- PSRAM record-buffer allocation failure: log + abort run (as today).

## Testing

Compile, flash, capture serial boot log; user taps the icon and adjusts
the slider to confirm audibility of beeps and playback.
