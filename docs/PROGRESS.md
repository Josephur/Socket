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

**WiFi confirmed working end-to-end** with real credentials in `Secrets.h`
-- connects and correctly falls through to the OFFLINE screen since
`SOCKET_PROVISIONING_URL` is still a placeholder (no PHP server exists yet).

**Added `src/core/RetryBackoff`:** the OFFLINE state was retrying WiFi
reconnect + provisioning fetch on every single `loop()` tick once WiFi
itself was up (only naturally throttled by the HTTP connect timeout).
Added a staggered schedule (3, 5, 10, 15, 30, 60s, then holds at 60s) so a
down/unreachable server won't get hammered once one exists. Verified via
serial log that retries are correctly spaced, not spammed.

**Added screen timeout (5 min idle -> backlight off).** New
`src/core/ActivityMonitor` centralizes "what counts as activity" in one
place: touch input, the BOOT button (GPIO35, standing in for a real wake
button until one exists), and any successful `AppState` transition -- which
covers connectivity changes (OFFLINE<->IDLE) and will automatically cover
wake-word detection once that's implemented, with no extra wiring needed.
`Socket.ino` polls this each `loop()` tick and toggles `DisplayDriver`'s
backlight; rendering itself keeps running underneath, only the backlight
cuts out.

**Bug found and fixed during verification:** the screen never timed out in
the first real test (left running 30+ min). Root cause: `AppStateMachine::
transitionTo()` treated *every* entry into OFFLINE as activity, including
re-entering OFFLINE from OFFLINE on each failed retry -- so as long as the
(nonexistent) provisioning server stayed unreachable, the device kept
waking itself back up every retry cycle (every <=60s via `RetryBackoff`)
and the countdown never survived long enough to fire. Fixed by only
counting a *genuine* state change into OFFLINE as activity. Verified via a
6-minute serial capture: `screen timed out after 5 min idle` fired exactly
once and stayed off through repeated offline retries afterward. User
confirmed visually -- screen went dark, touch woke it back up immediately.

**Toolchain note:** for testing timing-sensitive behavior like this, a
background serial capture (raw PowerShell `SerialPort`, run in the
background for N minutes, grep the log afterward) works well and avoids
needing to babysit a live monitor session.

**Next steps:** vendor the `es8311` component for real audio; spike the
wake-word research question; start scoping the PHP `/api/converse`
orchestration endpoint against the proposed wire format (see the open
question about OpenWebUI custom Agents/Models vs. client-supplied prompts
for how the LLM side of that endpoint should work); add commonly used
AI-related emoji/icons from FiraCode Nerd Font Mono to the LVGL UI
(avatar/status icons, idle screen) instead of plain text where appropriate;
consider a dedicated physical wake button instead of reusing BOOT (GPIO35)
long-term.

## Speaker/mic bring-up + test harness

Implemented real ES8311 (speaker/DAC) and ES7210 (mic/ADC) I2C register
init in `src/audio/codecs/` -- best-effort reconstructions from the widely
published ES8311 pattern (see each file's CONFIDENCE NOTE), not vendored or
datasheet-verified in this session. Wired into `AudioCodec::codecInit()`.

Built a dedicated test harness (`src/audio/AudioSelfTest.*`) with two
modes, toggled via `SOCKET_AUDIO_TEST_MODE` in `Socket.ino` (currently
**on** -- set to 0 to return to the normal app):
- An automated tone-detection self-test: plays a 1kHz tone out the speaker
  while simultaneously recording from the mic, comparing recorded loudness
  against a pre-tone silence baseline. No human required to get a signal.
- A live mic->speaker loopback that runs forever, for a human to test by
  talking into the mic and listening to the speaker.

Both subsystems skip WiFi/display/LVGL entirely in test mode -- pure audio
path, smaller/simpler binary (565KB vs 1.4MB), nothing else that could mask
or complicate results.

**Real results from flashing this to the device:**
- **ES8311 (speaker) chip ID register 0xFD read back `0x83`** -- this
  matches the value I expected from memory exactly, which is a genuine
  (if partial) confirmation that the ES8311 I2C link and my register
  reconstruction are probably in the right ballpark. Worth confirming by
  ear tomorrow (a 1kHz tone plays once during the self-test, right after
  boot).
- **ES7210 (mic) chip ID register 0xFD read back `0xFF`**, and both the
  baseline-silence and tone-playing RMS measurements came back exactly
  `0.00` -- not just quiet, but literally all-zero samples, which usually
  means the ADC isn't actually streaming real data over I2S (stuck in
  power-down, wrong format/slot config, or the 0xFD register guess simply
  isn't ES7210's chip ID at all -- unlike ES8311, I don't have strong
  confidence in ES7210's register map, see the file's CONFIDENCE NOTE).
  **This needs the real ES7210 datasheet to fix properly** -- not something
  to keep guessing at from memory; further blind attempts risk wasting
  time on wrong addresses.

**For tomorrow's manual test:** device is left flashed with
`SOCKET_AUDIO_TEST_MODE` on. Power it on and listen for a ~1 second 1kHz
tone shortly after boot (confirms speaker output). The subsequent live
loopback won't produce audible mic passthrough yet since the mic path
isn't capturing real data -- that's expected given the RMS=0 result above,
not a new failure. Serial log at 115200 baud shows the same self-test
numbers if you want to watch it boot.

**Next steps (in addition to earlier ones):** get the real ES7210
datasheet/register map (or find its actual public component source) rather
than continuing to guess; once mic capture produces nonzero RMS, rerun the
tone-detection self-test for a real pass/fail; set
`SOCKET_AUDIO_TEST_MODE` back to 0 once audio is confirmed working to
return to the normal app.

## Speaker/mic bring-up: resolved

Added a tap-to-play test tone (`AudioSelfTest::playTestTone`, wired to any
touch tap in `Socket.ino`) for on-demand testing since the boot-time tone
wasn't audible. User confirmed touch detection was working (taps logged
correctly, tone-generation code ran) but heard **no sound at all** --
pointing at the speaker output stage specifically, not touch/detection.

Rather than keep guessing at register values, pulled the actual public
Espressif drivers via `gh api`:
`github.com/espressif/esp-bsp/components/es8311` and `.../es7210`
(Apache-2.0). Two real bugs found by diffing against my earlier
reconstruction:

1. **ES8311 was missing the post-reset power-on write.** The real
   `es8311_init()` does reset (`REG00=0x1F` -> delay -> `REG00=0x00`) *then*
   `REG00=0x80` ("power-on command"). My version stopped after the `0x00`
   write -- the chip was never actually powered on.
2. **Wrong MCLK ratio entirely, affecting both codecs.** ES8311 and ES7210
   each ship their own MCLK-to-sample-rate coefficient table (real values,
   not reconstructed). Our I2S was using the default 256x multiple
   (6.144MHz MCLK @ 24kHz), which exists in ES8311's table but has **no
   24kHz entry at all** in ES7210's table -- explaining exactly why the mic
   returned all-zero samples while the speaker chip's ID read still
   succeeded. Fixed in `AudioCodec::i2sInit()` by setting
   `mclk_multiple = I2S_MCLK_MULTIPLE_512` (12.288MHz), the one MCLK value
   both tables agree on at 24kHz. Rewrote `Es8311.cpp`/`Es7210.cpp`'s
   register sequences to match the real drivers' values for that exact
   MCLK/rate combination (not the full dynamic table -- just the one row
   this board's fixed 24kHz config needs; see each file's header for the
   real source URLs and a note to look up a different row if the sample
   rate ever changes).

**Result after reflashing:** baseline (silence) RMS 28.09, tone-playing RMS
2614.71 (~93x jump) -- `AudioSelfTest` reports TONE DETECTED / SELF-TEST
PASSED. Both the speaker and mic paths are now confirmed physically
working via measured hardware data, not just register writes succeeding.

**Next steps:** try `AudioSelfTest::runLiveLoopbackForever` again now that
the mic path streams real data, for an actual voice loopback test; remove
the temporary tap-to-play test tone once satisfied; consider adding
`es8311_voice_fade`-style volume ramping later for less jarring TTS
playback; set `SOCKET_AUDIO_TEST_MODE` back to 0 (already done) and move on
to wiring real `AudioRecorder`/`AudioPlayer` usage into the conversation
flow.
