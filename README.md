# Socket 🦀

Arduino firmware for the **Waveshare ESP32-P4-WIFI6-Touch-LCD-4B** — a 4" round 720×720 IPS Smart 86 Box with ESP32-P4 RISC-V processor and ESP32-C6 WiFi 6 coprocessor.

## Hardware Overview

| Component | Detail |
|-----------|--------|
| MCU | ESP32-P4 (RISC-V dual-core 360MHz + single-core 40MHz LP) |
| WiFi/BLE | ESP32-C6 via SDIO (WiFi 6, BT 5 LE) |
| RAM | 32MB PSRAM (in-package) + 768KB HP L2MEM |
| Flash | 32MB NOR Flash |
| Display | 4" round IPS, 720×720, ST7703 (MIPI DSI 2-lane) |
| Touch | GT911 capacitive, 5-point, I2C |
| Audio | ES8311 codec + ES7210 echo cancellation, dual mics |
| PMU | AXP2101 |
| RTC | PCF85063 |
| IMU | QMI8658 (6-axis accel + gyro) |
| SD | TF slot (SDIO 3.0) |
| Camera | MIPI CSI 2-lane (15-pin) |
| USB | USB 2.0 OTG (Type-A) + USB-UART (Type-C) |
| IO | Expansion header 2.0mm pitch |

## Requirements

- **Arduino IDE 2.3.8** (or later)
- **ESP32 Arduino core** ≥ 3.2.0 (board: `ESP32P4 Dev Module`)
- **Required Libraries:**
  - `GFX_Library_for_Arduino` — graphical library
  - `lvgl` v9.3.0 — LVGL UI framework
  - `displays` — Waveshare I2C/touch/driver collection
  - `XPowersLib` — AXP2101 PMU driver
  - `SensorLib` — RTC/IMU sensor drivers

See [Getting Started](https://www.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-4B) on the Waveshare wiki.

## Project Structure

```
├── ci/                   CI pipeline examples (compile + lint)
├── examples/             Arduino example sketches
│   └── 01_HelloWorld/    First test — "Hello World" on display
├── lib/                  Custom libraries (as needed)
├── docs/
│   ├── PINOUT.md         Verified pin mapping
│   └── SETUP.md          Environment setup guide
├── .clang-format         C++ style enforcement
├── .clang-tidy           Static analysis config
└── README.md
```

## First Test

Open `examples/01_HelloWorld/HelloWorld.ino` in Arduino IDE 2.3.8, select `ESP32P4 Dev Module`, and upload.

> **Note:** CI workflows are in `ci/`. To enable them, copy into `.github/workflows/` — requires a GitHub token with `workflow` scope.
