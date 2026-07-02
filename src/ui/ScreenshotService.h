#pragma once

#include "DisplayDriver.h"

// On-demand screenshots over the existing serial/log connection, for remote
// debugging (human or AI agent) without any extra hardware or client app.
//
// Protocol: the host sends the line "SNAP". The device snapshots the DSI
// framebuffer (one ~1MB memcpy, a few ms) and then streams it back
// incrementally from loop() -- a bounded chunk per tick, so touch/LVGL/audio
// stay responsive throughout. Every screenshot line is prefixed "SS:" so it
// can be filtered out of the ordinary log stream with a string match:
//
//   SS:BEGIN <width> <height> RGB565RLE
//   SS:<base64 of RLE bytes>          (repeated; each line independently
//                                      base64-decodable)
//   SS:END <total RLE byte count>
//
// RLE format: repeated [count:u8][pixel:u16 little-endian] pairs, count
// 1..255, pixels in row-major order. UI screens are mostly flat color, so
// this typically shrinks the 1MB frame by 5-20x with ~15 lines of code.
// scripts/screenshot.ps1 is the reference host-side decoder (saves a PNG).
namespace ScreenshotService {

// Call once after DisplayDriver::begin() (needs the framebuffer to exist).
void begin(DisplayDriver &display);

// Call every loop() tick: watches serial RX for the SNAP command and, while
// a transfer is active, emits the next chunk.
void tick();

}  // namespace ScreenshotService
