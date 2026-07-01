# Socket — Verified Pin Mapping (ESP32-P4-WIFI6-Touch-LCD-4B)

> Source: pevin32/ESP32 (Xiaozhi voice assistant) board definition + Waveshare BSP

## Display — ST7703 (MIPI DSI)

| Signal | GPIO | Notes |
|--------|------|-------|
| DSI Data 0 | MIPI_DSI_DP0/DN0 | Lane 0 differential pair |
| DSI Data 1 | MIPI_DSI_DP1/DN1 | Lane 1 differential pair |
| DSI Clock | MIPI_DSI_CKP/CKN | Clock differential pair |
| LCD RST | GPIO 27 | Active low reset |
| Backlight | GPIO 26 | PWM, **inverted** (low = on) |
| DSI PHY LDO | Channel 3 | 2500mV, must be configured at firmware level |

**Display Timing:**
- Resolution: 720 × 720
- Pixel format: RGB565 (16-bit)
- DPI clock: 46 MHz
- H: 720 active, 20 hsync, 80 hbp, 80 hfp
- V: 720 active, 4 vsync, 12 vbp, 30 vfp
- MIPI DSI lanes: 2

## Touch — GT911 (I2C)

| Signal | GPIO | Notes |
|--------|------|-------|
| SDA | GPIO 7 | Shared with audio codec I2C |
| SCL | GPIO 8 | Shared with audio codec I2C |
| RST | GPIO 23 | Active low |
| INT | NC | Not connected |

- I2C bus: 1 (shared bus with ES8311/ES7210)
- Speed: 400 kHz
- Address: 0x14 (GT911 default)

## Audio — ES8311 + ES7210

| Signal | GPIO | Notes |
|--------|------|-------|
| I2C SDA | GPIO 7 | Shared with touch |
| I2C SCL | GPIO 8 | Shared with touch |
| I2S MCLK | GPIO 13 | Master clock |
| I2S BCLK | GPIO 12 | Bit clock |
| I2S WS | GPIO 10 | Word select / LR clock |
| I2S DOUT | GPIO 9 | Data out (to codec) |
| I2S DIN | GPIO 11 | Data in (from codec) |
| PA Enable | GPIO 53 | Amplifier power, active high |

- Sample rate: 24 kHz (configurable)
- ES8311 address: 0x18 (default)
- ES7210 address: 0x40 (default)

## Buttons

| Button | GPIO | Notes |
|--------|------|-------|
| BOOT | GPIO 35 | Pull low during power-on/reset for download mode |

## PMU — AXP2101

| Signal | GPIO/Pin | Notes |
|--------|----------|-------|
| SDA | GPIO 7 | Same I2C bus |
| SCL | GPIO 8 | Same I2C bus |

## RTC — PCF85063

| Signal | GPIO/Pin | Notes |
|--------|----------|-------|
| SDA | GPIO 7 | Same I2C bus |
| SCL | GPIO 8 | Same I2C bus |

## IMU — QMI8658

| Signal | GPIO/Pin | Notes |
|--------|----------|-------|
| SDA | GPIO 7 | Same I2C bus |
| SCL | GPIO 8 | Same I2C bus |

## SD Card — TF Slot (SDIO 3.0)

Connected via the P4's internal SDMMC peripheral. Pin mapping per ESP32-P4 reference design.

## USB

| Port | Type | Function |
|------|------|----------|
| USB-UART | Type-C | Power, flashing, serial debug -- wired to the P4 only |
| USB OTG | Type-C | USB 2.0 High Speed OTG -- also wired to the P4 only |

## ESP32-C6 co-processor flashing

Neither USB-C port reaches the C6 -- it has its own separate 2.54mm 4-pin
header ("For flashing firmware to the ESP32-C6 module" per Waveshare's
hardware description). Flashing it requires an external USB-TTL adapter
wired to that header, plus pulling C6_IO9 low at power-on to force the C6
into its own download mode (independent of the P4's BOOT button). Under
normal operation the C6 just runs Espressif's ESP-Hosted slave firmware and
isn't something this project's firmware touches.

> **Note on I2C Bus:** Touch, audio codec, PMU, RTC, and IMU all share I2C bus 1 (GPIO 7/8). Each has a unique address so there's no conflict.
