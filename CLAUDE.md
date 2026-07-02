# Socket — agent guide

AI-assistant firmware for the Waveshare ESP32-P4-WIFI6-Touch-LCD-4B
(720x720 MIPI-DSI touch screen, ES8311/ES7210 audio codecs). `Socket.ino`
wires modules together; logic lives under `src/`. See README.md for layout.

## Taking a screenshot of the device's screen

When asked to "take a screenshot" (or you want to see the UI yourself):

```powershell
./scripts/screenshot.ps1
```

- Saves a PNG to `screenshots/screenshot-<timestamp>.png` — Read that file
  to view it. Screenshots accumulate; never delete them, the user cleans up
  manually.
- Requires the board on COM3 (default; `-Port COMx` to override) and the
  ScreenshotService firmware flashed (any build since July 2026).
- The script doubles as a serial monitor: device log lines print to the
  console while it waits, so you also get recent logs for free.
- Opening the COM port sometimes reboots the board (auto-reset circuit
  race). The script retries `SNAP` automatically; expect ~3-15s total.

## Build & flash

```powershell
./arduino-cli/arduino-cli.exe compile --fqbn esp32:esp32:esp32p4 --board-options "PSRAM=enabled,FlashMode=qio,FlashSize=32M,PartitionScheme=app5M_fat24M_32MB" .
./arduino-cli/arduino-cli.exe upload -p COM3 --fqbn esp32:esp32:esp32p4 --board-options "PSRAM=enabled,FlashMode=qio,FlashSize=32M,PartitionScheme=app5M_fat24M_32MB" .
```

Boot takes ~3s and runs an audio self-test (tone + mic check) before
`loop()` starts.

## Reading serial from scripts

Assert **both** DTR and RTS before opening the port — with DTR deasserted
the device never receives host->device bytes, and asserting both together
keeps the auto-reset circuit neutral. 115200 baud. PowerShell gotcha:
`-shl` keeps the left operand's byte type (`[byte]$b -shl 8` wraps to 0);
cast `[int]` first.

## Hardware gotchas that have burned time before

- **Audio "not working" is usually volume.** The boot self-test passing
  ("TONE DETECTED") does NOT prove audible output — the mic sits next to
  the speaker with gain maxed and hears tones a human can't. The ES8311
  DAC volume register is in dB; `Es8311::setVolume()` maps 0-100% onto
  -30..+32dB (0% = mute). Test audio with the on-screen speaker button
  (beep, record 3s, playback) and volume slider (release = beep at that
  level).
- **Touch must be polled with state caching.** The GT911 posts samples at
  ~100Hz and the vendored driver zeroes its state on every read; multiple
  pollers or polling faster than the scan rate sees phantom releases.
  `TouchDriver::read()` handles this — don't add raw
  `esp_lcd_touch_read_data()` calls elsewhere.
- **No blocking work in `loop()`.** Touch/LVGL/serial polling all run
  there; long work goes on a FreeRTOS task (see networkAttemptTask /
  AudioTestPanel's demo task). Watch for "slow loop() tick" warnings in
  the log.
