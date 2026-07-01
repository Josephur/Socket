# Socket 🦀

AI assistant firmware for the **Waveshare ESP32-P4-WIFI6-Touch-LCD-4B** (rectangular panel variant, no Ethernet/relay, MIPI-CSI camera connector present). Built for Stack-Tech's shop floor: wake-word activated voice conversation with a self-hosted Gemma model (via OpenWebUI) and self-hosted TTS, orchestrated through a Stack-Tech PHP server. See the project plan for full architecture/decision background.

## Hardware Overview

| Component | Detail |
|-----------|--------|
| MCU | ESP32-P4 (RISC-V dual-core 360MHz + single-core 40MHz LP) |
| WiFi/BLE | ESP32-C6-MINI co-processor over SDIO (WiFi 6, BT 5 LE), transparent via ESP-Hosted |
| RAM | 32MB PSRAM (in-package) + 768KB HP L2MEM |
| Flash | 32MB NOR Flash |
| Display | 720×720 IPS, ST7703 (MIPI DSI 2-lane) |
| Touch | GT911 capacitive, 5-point, I2C (no INT pin wired — polled) |
| Audio | ES8311 codec (DAC) + ES7210 echo-cancelling ADC, dual mics |
| PMU | AXP2101 |
| RTC | PCF85063 |
| IMU | QMI8658 (6-axis accel + gyro) |
| SD | TF slot (SDIO 3.0) |
| Camera | MIPI CSI 2-lane (15-pin), no sensor installed yet |
| USB | Two USB-C ports: OTG 2.0 HS + USB-to-UART (programming/debug), both wired to the P4 only |
| C6 flashing | Separate 2.54mm 4-pin header — not reachable via either USB-C port, needs an external USB-TTL adapter |

## Requirements

- **Arduino IDE 2.3.8** (or later), or `arduino-cli`
- **ESP32 Arduino core** 3.3.10+ (board: `ESP32P4 Dev Module`) — verified working; earlier 3.2.x/3.3.0 cores may not compile the GFX library version pinned below
- **Required libraries:** pinned in [`libraries.txt`](libraries.txt) — see [docs/SETUP.md](docs/SETUP.md) for the one-command install
- **Board options:** PSRAM=enabled, FlashMode=qio, FlashSize=16M, PartitionScheme=app3M_fat9M_16MB

## Project Structure

```
Socket.ino                 setup()/loop() only — wires modules, drives the state machine
src/
  core/                    AppState state machine, Logger
  config/                  DeviceIdentity, ProvisioningClient, RuntimeConfig, Secrets.h (gitignored)
  network/                 WiFiManager, ConversationClient, OtaUpdater (stub)
  audio/                   AudioCodec, WakeWordDetector (stub), AudioRecorder, AudioPlayer
  ui/                      DisplayDriver, TouchDriver, LvglBridge, screens/
    board/                 Vendored Waveshare GT911/I2C/display-init glue (not a Library Manager package)
  camera/                  CameraService interface stub (no camera hardware yet)
libraries.txt              Pinned Library Manager dependencies
scripts/                   install-libs.sh / .ps1 — installs libraries.txt + places lv_conf.h
docs/                      Pin mapping, setup guide, TTS endpoint reference, vendor datasheets/examples
ci/                        Compile + lint CI pipeline (copy into .github/workflows/ to enable)
```

## Getting Started

See [docs/SETUP.md](docs/SETUP.md) for the full walkthrough (library install, `Secrets.h`, board settings). Short version:

```bash
arduino-cli lib update-index
./scripts/install-libs.sh          # or scripts\install-libs.ps1 on Windows
cp src/config/Secrets.h.example src/config/Secrets.h   # then fill in your WiFi + provisioning URL
arduino-cli compile --fqbn esp32:esp32:esp32p4 \
  --board-options "PSRAM=enabled,FlashMode=qio,FlashSize=32M,PartitionScheme=app5M_fat24M_32MB" .
```

## Status

Core scaffold is in place and compiles clean (display/touch/LVGL, WiFi + NVS credentials, HTTPS provisioning, REST conversation client, I2S plumbing). Two pieces are explicitly stubbed pending further work — see their headers for details:

- **`src/audio/AudioCodec.h`** — I2S peripheral setup is real; the ES8311/ES7210 codec register configuration needs the `es8311` ESP-IDF component vendored in.
- **`src/audio/WakeWordDetector.h`** — no on-device wake-word implementation yet; this is an open research spike (see the project plan's "Key Risks").

> **Note:** CI workflows are in `ci/`. To enable them, copy into `.github/workflows/` — requires a GitHub token with `workflow` scope.
