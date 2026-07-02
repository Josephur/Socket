#pragma once

#include <lvgl.h>

// Full-color emoji icon set: 17 Twemoji glyphs (Twitter's open-source emoji
// set, CC-BY 4.0, https://github.com/jdecked/twemoji) rasterized to 32x32
// PNG with resvg and converted to LVGL image descriptors with LVGL's own
// scripts/LVGLImage.py (--cf RGB565A8 --compress NONE -- deliberately
// uncompressed; see docs/ICONS.md for why compressed glyph data crashed the
// earlier monochrome font on this LVGL version).
//
// Unlike AiIcons.h (single-tint font glyphs drawn via lv_label), these are
// full-color raster images drawn via lv_image_create()/lv_image_set_src():
//
//   lv_obj_t *img = lv_image_create(parent);
//   lv_image_set_src(img, &AiEmoji::kRobot);
//
// To add/change icons: find the codepoint's Twemoji SVG under
// https://github.com/jdecked/twemoji/tree/main/assets/svg (filename is the
// lowercase hex codepoint), rasterize with resvg (`resvg -w 32 -h 32 in.svg
// out.png`), then run LVGLImage.py --ofmt C --cf RGB565A8 --compress NONE
// -o <dir> --name emoji_<name> out.png, fix its lvgl.h include (see below),
// and add an extern + alias here.

extern const lv_image_dsc_t emoji_robot;
extern const lv_image_dsc_t emoji_brain;
extern const lv_image_dsc_t emoji_sparkles;
extern const lv_image_dsc_t emoji_speech_balloon;
extern const lv_image_dsc_t emoji_thought_balloon;
extern const lv_image_dsc_t emoji_magic_wand;
extern const lv_image_dsc_t emoji_microphone;
extern const lv_image_dsc_t emoji_speaker;
extern const lv_image_dsc_t emoji_speaker_muted;
extern const lv_image_dsc_t emoji_signal;
extern const lv_image_dsc_t emoji_cloud;
extern const lv_image_dsc_t emoji_gear;
extern const lv_image_dsc_t emoji_lightning;
extern const lv_image_dsc_t emoji_check;
extern const lv_image_dsc_t emoji_cross;
extern const lv_image_dsc_t emoji_warning;
extern const lv_image_dsc_t emoji_battery;

namespace AiEmoji {

constexpr const lv_image_dsc_t *kRobot = &emoji_robot;                    // U+1F916
constexpr const lv_image_dsc_t *kBrain = &emoji_brain;                    // U+1F9E0
constexpr const lv_image_dsc_t *kSparkles = &emoji_sparkles;              // U+2728
constexpr const lv_image_dsc_t *kSpeechBalloon = &emoji_speech_balloon;   // U+1F4AC
constexpr const lv_image_dsc_t *kThoughtBalloon = &emoji_thought_balloon; // U+1F4AD
constexpr const lv_image_dsc_t *kMagicWand = &emoji_magic_wand;           // U+1FA84
constexpr const lv_image_dsc_t *kMicrophone = &emoji_microphone;         // U+1F3A4
constexpr const lv_image_dsc_t *kSpeaker = &emoji_speaker;                // U+1F50A
constexpr const lv_image_dsc_t *kSpeakerMuted = &emoji_speaker_muted;     // U+1F507
constexpr const lv_image_dsc_t *kSignal = &emoji_signal;                  // U+1F4F6
constexpr const lv_image_dsc_t *kCloud = &emoji_cloud;                    // U+2601
constexpr const lv_image_dsc_t *kGear = &emoji_gear;                      // U+2699
constexpr const lv_image_dsc_t *kLightning = &emoji_lightning;            // U+26A1
constexpr const lv_image_dsc_t *kCheck = &emoji_check;                    // U+2705
constexpr const lv_image_dsc_t *kCross = &emoji_cross;                    // U+274C
constexpr const lv_image_dsc_t *kWarning = &emoji_warning;                // U+26A0
constexpr const lv_image_dsc_t *kBattery = &emoji_battery;                // U+1F50B

}  // namespace AiEmoji
