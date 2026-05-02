# Setting Up the Arduino IDE Environment

## 1. Install Arduino IDE 2.3.8

Download from [arduino.cc](https://www.arduino.cc/en/software).

## 2. Install ESP32 Board Support

1. Open Arduino IDE → File → Preferences
2. Add to **Additional Boards Manager URLs**:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Tools → Board → Boards Manager → search "esp32" → install **esp32 by Espressif Systems** ≥ v3.2.0

## 3. Board Configuration for ESP32-P4-WIFI6-Touch-LCD-4B

| Setting | Value |
|---------|-------|
| Board | `ESP32P4 Dev Module` |
| PSRAM | Enabled |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB |
| USB CDC On Boot | Disabled (use UART port) |
| Partition Scheme | 16M Flash (3MB) |

## 4. Install Required Libraries

Install via **Tools → Manage Libraries** or manually from the demo package:

| Library | Version | Install Method |
|---------|---------|----------------|
| GFX_Library_for_Arduino | ≥1.6.0 | Search "GFX" in Library Manager |
| lvgl | 9.3.0 | Search "lvgl" (then copy demos to src/) |
| displays | — | Offline (from Waveshare demo package) |
| SensorLib | — | Search "SensorLib" |
| XPowersLib | — | Search "XPowersLib" |
| Mylibrary | — | Offline (Waveshare board definitions) |

## 5. Verify Installation

Open `examples/01_HelloWorld` and upload to verify the display works.

## 6. Troubleshooting

- **Can't flash:** Hold BOOT, press RESET, release BOOT
- **No serial output:** Check USB CDC On Boot setting matches port type (UART vs USB)
- **Screen stays blank:** Verify PSRAM is enabled in board config
- **First flash fails:** Enable "Erase All Flash Before Sketch Upload" once, then disable
