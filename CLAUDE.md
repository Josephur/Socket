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
- It captures the screen as-is, without rebooting the board (see the
  serial section below for why that matters). Expect ~3-10s total.

## Build & flash

```powershell
./arduino-cli/arduino-cli.exe compile --fqbn esp32:esp32:esp32p4 --board-options "PSRAM=enabled,FlashMode=qio,FlashSize=32M,PartitionScheme=app5M_fat24M_32MB" .
./arduino-cli/arduino-cli.exe upload -p COM3 --fqbn esp32:esp32:esp32p4 --board-options "PSRAM=enabled,FlashMode=qio,FlashSize=32M,PartitionScheme=app5M_fat24M_32MB" .
```

Boot takes ~3s and runs an audio self-test (tone + mic check) before
`loop()` starts.

## Reading serial from scripts

Leave DTR and RTS both **deasserted** (the .NET SerialPort defaults) when
opening COM3. The auto-reset circuit reboots the chip if RTS goes high
while DTR is low, and reboots it INTO THE BOOTLOADER ("waiting for
download") if DTR is high while RTS is low during a reset — and Windows
applies the lines non-atomically on open, so any asserted combination can
transiently hit one of those. Both-deasserted is safe in every ordering;
host->device data flows fine without DTR (send commands with a short
retry to cover port settling). To deliberately reset the board: open with
RTS asserted + DTR deasserted, hold ~150ms, deassert RTS. To recover from
accidental bootloader mode: same reset pulse. 115200 baud. PowerShell
gotcha: `-shl` keeps the left operand's byte type (`[byte]$b -shl 8`
wraps to 0); cast `[int]` first.

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
