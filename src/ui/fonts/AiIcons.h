#pragma once

#include <lvgl.h>

// Custom LVGL font subset: 20 icon glyphs pulled from the real FiraCode
// Nerd Font Mono TTF (downloaded from
// https://github.com/ryanoasis/nerd-fonts/raw/master/patched-fonts/FiraCode/Regular/FiraCodeNerdFontMono-Regular.ttf,
// MIT license) via `lv_font_conv` (`npx lv_font_conv --font ... --format
// lvgl`). Every codepoint below was verified against the real font's cmap
// table with fonttools before being included -- see docs/PROGRESS.md for
// the exact verification command -- so these are confirmed-correct
// codepoints, not guessed from memory of the Nerd Font cheat sheet.
//
// To add/change icons: find the real codepoint with fonttools
// (`f.getBestCmap()` on the TTF), regenerate lv_font_ai_icons_24.c with
// lv_font_conv listing every codepoint via `-r 0x...`, and update this
// enum to match.
extern const lv_font_t lv_font_ai_icons_24;

namespace AiIcons {

// UTF-8 encoded strings for each glyph, ready to hand to lv_label_set_text
// with style_set_text_font(&lv_font_ai_icons_24) applied to the label.
constexpr const char *kRobot = "\xEE\xB8\x8D";        // U+EE0D fa-robot
constexpr const char *kBrain = "\xEE\xBA\x9C";        // U+EE9C fa-brain
constexpr const char *kMicrochip = "\xEF\x8B\x9B";    // U+F2DB fa-microchip
constexpr const char *kSparkle = "\xEE\xB0\x90";      // U+EC10 cod-sparkle
constexpr const char *kMagicWand = "\xEF\x83\x90";    // U+F0D0 fa-wand_magic
constexpr const char *kComment = "\xEF\x81\xB5";      // U+F075 fa-comment
constexpr const char *kComments = "\xEF\x82\x86";     // U+F086 fa-comments
constexpr const char *kMicrophone = "\xEF\x84\xB0";   // U+F130 fa-microphone
constexpr const char *kVolumeUp = "\xEF\x80\xA8";     // U+F028 fa-volume_up
constexpr const char *kVolumeOff = "\xEF\x80\xA6";    // U+F026 fa-volume_off
constexpr const char *kWifi = "\xEF\x87\xAB";         // U+F1EB fa-wifi
constexpr const char *kBluetooth = "\xEF\x8A\x93";    // U+F293 fa-bluetooth
constexpr const char *kCloud = "\xEF\x83\x82";        // U+F0C2 fa-cloud
constexpr const char *kDatabase = "\xEF\x87\x80";     // U+F1C0 fa-database
constexpr const char *kGear = "\xEF\x80\x93";         // U+F013 fa-gear
constexpr const char *kBolt = "\xEF\x83\xA7";         // U+F0E7 fa-flash
constexpr const char *kCheck = "\xEF\x80\x8C";        // U+F00C fa-check
constexpr const char *kClose = "\xEF\x80\x8D";        // U+F00D fa-xmark
constexpr const char *kWarning = "\xEF\x81\xB1";      // U+F071 fa-warning
constexpr const char *kBatteryFull = "\xEF\x89\x80";  // U+F240 fa-battery_full

}  // namespace AiIcons
