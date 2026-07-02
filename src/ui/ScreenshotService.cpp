#include "ScreenshotService.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <mbedtls/base64.h>

#include <cstring>

#include "../core/Logger.h"

namespace {
constexpr const char *kTag = "Screenshot";

// RLE bytes encoded per loop() tick. Deliberately small: at 115200 baud
// the serial line drains ~11.5 bytes/ms, and Serial.write blocks once the
// TX buffer is full, so the per-tick line length is what bounds the
// per-tick stall. 96 RLE bytes -> ~132-byte line -> ~11ms worst case (a
// first cut at 1KB/tick stalled loop() 125ms per tick for the whole
// transfer, visibly freezing touch). emitChunk() additionally skips a tick
// entirely when the TX buffer hasn't drained yet, keeping the common case
// non-blocking. A typical UI frame (~25KB RLE) still completes in a few
// seconds -- the wire is the bottleneck either way.
constexpr size_t kChunkRleBytes = 96;

DisplayDriver *g_display = nullptr;
uint16_t *g_snapshot = nullptr;  // PSRAM copy, allocated once on first SNAP
size_t g_totalPixels = 0;
size_t g_pixelPos = 0;
size_t g_rleBytesSent = 0;
bool g_streaming = false;

char g_cmdBuf[16];
size_t g_cmdLen = 0;

void startTransfer() {
  if (g_streaming) return;  // already in progress, ignore

  uint16_t *fb = g_display ? g_display->framebuffer() : nullptr;
  if (!fb) {
    Serial.println("SS:ERR no framebuffer");
    return;
  }

  g_totalPixels =
      static_cast<size_t>(g_display->width()) * g_display->height();
  if (!g_snapshot) {
    g_snapshot = static_cast<uint16_t *>(heap_caps_malloc(
        g_totalPixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
  }
  if (!g_snapshot) {
    Serial.println("SS:ERR snapshot alloc failed");
    return;
  }

  // One-shot copy so the streamed image is a consistent frame even though
  // LVGL keeps rendering into the live framebuffer during the transfer.
  memcpy(g_snapshot, fb, g_totalPixels * sizeof(uint16_t));

  g_pixelPos = 0;
  g_rleBytesSent = 0;
  g_streaming = true;
  Serial.printf("SS:BEGIN %u %u RGB565RLE\n", g_display->width(),
                g_display->height());
}

void emitChunk() {
  // Worst-case line is 3 + ceil((kChunkRleBytes+2)/3)*4 + 1 bytes; only
  // emit when the TX buffer can absorb all of it, so Serial.write returns
  // without blocking and loop() keeps its normal tick rate. The line goes
  // out in the background while we wait for the next slot.
  constexpr size_t kMaxLineBytes = 3 + ((kChunkRleBytes + 2 + 2) / 3) * 4 + 1;
  if (Serial.availableForWrite() < static_cast<int>(kMaxLineBytes)) return;

  // [count][pixel lo][pixel hi] triplets; +3 slack so a triplet never
  // splits across chunks (each line must stay independently decodable).
  static uint8_t rle[kChunkRleBytes + 3];
  size_t rleLen = 0;

  while (rleLen + 3 <= sizeof(rle) && g_pixelPos < g_totalPixels) {
    uint16_t pixel = g_snapshot[g_pixelPos];
    size_t run = 1;
    while (run < 255 && g_pixelPos + run < g_totalPixels &&
           g_snapshot[g_pixelPos + run] == pixel) {
      run++;
    }
    rle[rleLen++] = static_cast<uint8_t>(run);
    rle[rleLen++] = pixel & 0xFF;
    rle[rleLen++] = pixel >> 8;
    g_pixelPos += run;
  }

  // Assemble the whole line ("SS:" + base64 + "\n") in one buffer and write
  // it with a single call -- background tasks (network retries) also log to
  // Serial, and a single write keeps this line atomic against those.
  static char line[3 + ((sizeof(rle) + 2) / 3) * 4 + 2];
  memcpy(line, "SS:", 3);
  size_t b64Len = 0;
  mbedtls_base64_encode(reinterpret_cast<unsigned char *>(line + 3),
                        sizeof(line) - 4, &b64Len, rle, rleLen);
  line[3 + b64Len] = '\n';
  Serial.write(line, 3 + b64Len + 1);
  g_rleBytesSent += rleLen;

  if (g_pixelPos >= g_totalPixels) {
    g_streaming = false;
    Serial.printf("SS:END %u\n", static_cast<unsigned>(g_rleBytesSent));
    Logger::info(kTag, ("screenshot sent, " + String(g_rleBytesSent) +
                         " RLE bytes")
                            .c_str());
  }
}

void pollCommand() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      g_cmdBuf[g_cmdLen] = '\0';
      if (g_cmdLen > 0 && strcmp(g_cmdBuf, "SNAP") == 0) {
        startTransfer();
      }
      g_cmdLen = 0;
    } else if (g_cmdLen < sizeof(g_cmdBuf) - 1) {
      g_cmdBuf[g_cmdLen++] = c;
    } else {
      g_cmdLen = 0;  // overlong garbage line, drop it
    }
  }
}
}  // namespace

namespace ScreenshotService {

void begin(DisplayDriver &display) { g_display = &display; }

void tick() {
  pollCommand();
  if (g_streaming) emitChunk();
}

}  // namespace ScreenshotService
