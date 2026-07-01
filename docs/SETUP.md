# Setting Up the Arduino IDE Environment

## 1. Install Arduino IDE 2.3.8

Download from [arduino.cc](https://www.arduino.cc/en/software).

## 2. Install ESP32 Board Support

1. Open Arduino IDE → File → Preferences
2. Add to **Additional Boards Manager URLs**:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Tools → Board → Boards Manager → search "esp32" → install **esp32 by Espressif Systems** — 3.3.10 is verified working with this project's pinned library versions

## 3. Board Configuration for ESP32-P4-WIFI6-Touch-LCD-4B

| Setting | Value |
|---------|-------|
| Board | `ESP32P4 Dev Module` |
| PSRAM | Enabled |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB |
| USB CDC On Boot | Disabled (use UART port) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) — `app3M_fat9M_16MB` |

## 4. Install Required Libraries

Every dependency is pinned in `libraries.txt` at the repo root — this is the
single source of truth (also used by `ci/compile.yml`), so run the install
script instead of installing libraries by hand:

```powershell
# Windows
arduino-cli lib update-index
.\scripts\install-libs.ps1
```

```bash
# macOS/Linux
arduino-cli lib update-index
./scripts/install-libs.sh
```

The Waveshare GT911/I2C/display-init glue code is **not** a Library Manager
package — it's vendored directly into `src/ui/board/` (ported from
`docs/Arduino/libraries/displays`), so there's nothing to install for it.

## 5. Configure secrets and verify installation

1. Copy `src/config/Secrets.h.example` to `src/config/Secrets.h` and fill in
   your WiFi credentials and provisioning URL (this file is gitignored).
2. Open `Socket.ino` (the repo root) in Arduino IDE, select `ESP32P4 Dev
   Module` with the board settings above, and upload. The device should
   connect to WiFi, attempt provisioning, and show either the idle screen or
   an offline indicator.

Or from the command line, using the same board options CI uses:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32p4 \
  --board-options "PSRAM=enabled,FlashMode=qio,FlashSize=32M,PartitionScheme=app5M_fat24M_32MB" \
  .
```

## 6. Troubleshooting

- **Can't flash:** Hold BOOT, press RESET, release BOOT
- **No serial output:** Check USB CDC On Boot setting matches port type (UART vs USB)
- **Screen stays blank:** Verify PSRAM is enabled in board config
- **First flash fails:** Enable "Erase All Flash Before Sketch Upload" once, then disable
