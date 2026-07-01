# Progress Notes

Running log of what's been done, in case a session gets picked up cold later.
See the project plan (architecture/decisions) for the "why" behind the design
-- this file is the "what's actually been done" record.

## 2026-06-30 -- Initial firmware scaffold + first flash

**Architecture locked in** (see project plan for full detail): wake-word
activated voice assistant, on-device VAD/wake-word only, everything else
(Gemma LLM via OpenWebUI, TTS via F5-TTS/XTTSv2) orchestrated server-side
behind a Stack-Tech PHP `/api/converse` endpoint. REST per-utterance, not
WebSocket. Config fetched once at boot via HTTPS, keyed by chip ID.

**Built:**
- Modular `src/` tree (`core/`, `config/`, `network/`, `audio/`, `ui/`,
  `camera/`) wired together by `Socket.ino`'s `AppState` state machine
  (`BOOT -> PROVISIONING -> IDLE -> LISTENING -> THINKING -> SPEAKING`, with
  `OFFLINE` reachable from anywhere).
- Display/touch/LVGL stack ported from the proven `docs/Arduino/examples/LVGLV9_Arduino`
  example -- `src/ui/board/` vendors the Waveshare GT911/I2C/display-init glue
  verbatim (it's not a Library Manager package); `DisplayDriver`,
  `TouchDriver`, and `LvglBridge` wrap it into this project's module
  boundaries.
- WiFi via NVS-stored credentials (`WiFiManager`), HTTPS provisioning
  (`ProvisioningClient`), and the `/api/converse` REST client
  (`ConversationClient`) with a documented length-prefixed binary response
  format (JSON header + raw audio, to avoid base64 overhead) -- this wire
  format is a *proposal*, not yet agreed with whoever builds the PHP side.
- `libraries.txt` + `scripts/install-libs.{sh,ps1}` as the single source of
  truth for Library Manager dependencies (also used by `ci/compile.yml`).

**Explicitly stubbed, not faked** (see each header's comments):
- `AudioCodec` -- I2S peripheral setup is real and uses the board's actual
  pins; ES8311/ES7210 codec register configuration needs the `es8311`
  ESP-IDF component vendored in (didn't want to hand-write codec registers
  from memory -- wrong register writes can silently produce garbage audio).
- `WakeWordDetector` -- no wake-word engine implementation; open research
  spike (no confirmed off-the-shelf Arduino wake-word library for ESP32-P4
  yet). Touch-button activation is the fallback if this doesn't pan out.
- `CameraService` -- interface-only stub, no camera hardware yet.

**Hardware findings from this session** (corrected stale docs where found):
- Board is the rectangular Waveshare ESP32-P4-WIFI6-Touch-LCD-4B with the
  camera header + rear I/O -- *not* the Ethernet/RS485/relay variant (that's
  a separate "86 Panel Bottom Board" expansion, confirmed via Waveshare's
  hardware description page).
- Real flash chip is **32MB**, not the 16MB assumed in the original
  README/SETUP docs -- fixed all board-options references
  (`FlashSize=32M,PartitionScheme=app5M_fat24M_32MB`).
- `GFX Library for Arduino@1.6.0` (originally pinned) does NOT compile
  against ESP32 Arduino core 3.3.10 (`spiFrequencyToClockDiv` signature
  changed) -- bumped to `1.6.6`, which does. Verified via a real
  `arduino-cli compile`.
- `HTTPClient::getStreamPtr()` on core 3.3.10 returns a type that needs
  `WiFiClient.h`/`WiFi.h` explicitly included to name -- worked around with
  `auto *stream = ...` in `ConversationClient.cpp` instead.
- The ESP32-C6 co-processor is **not** reachable from either of the board's
  two USB-C ports (one OTG, one UART -- both wired only to the P4). It has
  its own separate 2.54mm 4-pin header for flashing, requiring an external
  USB-TTL adapter and manually pulling `C6_IO9` low at power-on. Under
  normal operation it just runs Espressif's ESP-Hosted slave firmware and
  isn't something this project touches -- so it wasn't backed up (only the
  P4 was, see below), on the reasoning that it's a replaceable, known
  component rather than unique factory data.

**First real hardware interaction:**
- Backed up the factory-flashed image before touching anything:
  `backups/factory_esp32p4_e8f60ae0f07f_32MB.bin` (full 32MB flash dump via
  `esptool read_flash`, gitignored -- it's a large binary, not repo content).
- Compiled and uploaded this scaffold to the device on COM3. First boot
  showed the screen flashing on/off -- turned out to be a real crash-reboot
  loop, not cosmetic. See below for root cause.

**Crash-reboot loop: root cause found and fixed.** Added verbose
entry/exit `Logger` calls through every init function (`DisplayDriver`,
`TouchDriver`, `LvglBridge`, `AudioCodec`, `Socket.ino`'s `setup()`) plus a
`Logger::flush()` before every risky call, specifically so a crash's last
buffered log line wouldn't be lost. Captured the actual serial log via a
raw PowerShell `System.IO.Ports.SerialPort` read (arduino-cli's own
`monitor` command mysteriously returned zero bytes across several attempts
and syntaxes on this machine -- unresolved, but the direct .NET read worked
fine, so that's the fallback if `arduino-cli monitor` misbehaves again).

The log showed display, touch, LVGL, and audio I2S all initializing
successfully -- the crash was:
```
E (2303) HS_MP: mempool create failed: no mem
assert failed: transport_drv_common_mempool_create transport_drv.c:283
```
This is the exact "WiFi + display resource contention" risk flagged in the
project plan, now confirmed concretely: `WiFi.begin()` starts the
ESP-Hosted SDIO transport to the C6, which needs its own DMA-capable memory
pool -- but LVGL's two draw buffers (`MALLOC_CAP_DMA`, originally 720x50px
= ~144KB combined) had already eaten too much of the ~226KB internal
DMA-capable heap by the time WiFi tried to allocate. **Fix:** shrank
`kDrawBufHeight` in `LvglBridge.cpp` from 50 to 16 rows. Reflashed and
confirmed via a fresh serial capture: no crash, clean boot through to a
normal WiFi connect timeout (expected -- `Secrets.h` still has placeholder
fake credentials) and the fallback logic running as designed. Added a
`heap_caps_get_free_size(MALLOC_CAP_DMA)` log line before WiFi init for
future tuning if the buffer size needs raising again.

**Toolchain note:** `arduino-cli` (v1.5.1) lives at `arduino-cli/` in the
repo root (gitignored -- it's a 36MB binary the user downloaded locally, not
repo content). ESP32 core 3.3.10 and all `libraries.txt` dependencies are
installed to the default Arduino15/`Documents/Arduino` locations on this
machine. `arduino-cli monitor -p COM3` produced zero bytes across multiple
syntax attempts even on a fresh reset; a raw PowerShell serial read worked
immediately -- worth trying that fallback before assuming the board itself
is silent.

**Next steps:** fill in real WiFi credentials + provisioning URL in
`Secrets.h` and confirm the device actually reaches the IDLE screen; vendor
the `es8311` component for real audio; spike the wake-word research
question; start scoping the PHP `/api/converse` orchestration endpoint
against the proposed wire format.
