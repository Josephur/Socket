# Icon sets

Socket has two separate icon sets for its LVGL UI, built for different
purposes. Both are currently only shown on `OfflineScreen` as a test row
(no permanent UI placement has been decided yet) -- see
`docs/superpowers/specs/2026-07-01-color-emoji-icons-design.md` for the
design history.

## 1. `AiIcons` -- monochrome Nerd Font glyphs

`src/ui/fonts/AiIcons.h` + `src/ui/fonts/lv_font_ai_icons_24.c`.

- **Source**: FiraCode Nerd Font Mono (patched font from
  https://github.com/ryanoasis/nerd-fonts, MIT license).
- **What it is**: a 20-glyph LVGL font subset. Every glyph is a single-tint
  alpha bitmap -- LVGL renders it like text and colors it with whatever
  `lv_obj_set_style_text_color()` you apply, same as normal text.
- **Use it when**: you want an icon that should match a text color (status
  bar, inline with a label) or only needs one color.
- **How to use**:
  ```cpp
  #include "../fonts/AiIcons.h"
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, AiIcons::kRobot);
  lv_obj_set_style_text_font(label, &lv_font_ai_icons_24, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(0xCCCCCC), 0);
  ```
- **Regenerating/extending**: download the real TTF, verify the codepoint
  you want against its actual cmap table with `fonttools` (do not trust a
  Nerd Font cheat sheet from memory -- codepoints vary by patched-font
  build; e.g. `fa-robot` is U+EE0D in this build, not the commonly-cited
  U+F544), then regenerate with:
  ```
  npx lv_font_conv --font FiraCodeNerdFontMono-Regular.ttf \
    -r 0x<codepoint> [-r 0x<codepoint> ...] \
    --size 24 --bpp 4 --no-compress --format lvgl \
    --lv-font-name lv_font_ai_icons_24 -o lv_font_ai_icons_24.c
  ```
  **`--no-compress` is required.** lv_font_conv defaults to RLE-compressed
  glyph bitmaps, but its generated code only wires up the decompression
  cache struct for `LVGL_VERSION_MAJOR == 8` -- on LVGL 9 (what this
  project uses) that leaves `.cache` unset, so every glyph lookup silently
  returns null and LVGL's software rasterizer crashes dereferencing it.
  This crashed the device on every single boot the first time this icon
  set was actually rendered (looked like the screen "blinking" from the
  outside -- it was a repeated panic+reboot loop). Regenerating without
  compression fixed it. Always regenerate with `--no-compress`.
  The generated file's `#ifdef LV_LVGL_H_INCLUDE_SIMPLE ... #else
  #include "lvgl/lvgl.h" #endif` block also needs replacing with a plain
  `#include <lvgl.h>` to match how this project includes LVGL everywhere
  else.

## 2. `AiEmoji` -- full-color Twemoji images

`src/ui/icons/AiEmoji.h` + `src/ui/icons/emoji_*.c` (one file per icon).

- **Source**: Twemoji, Twitter's open-source emoji set
  (https://github.com/jdecked/twemoji -- the maintained continuation after
  Twitter archived the original repo), CC-BY 4.0.
- **What it is**: 17 full-color raster images (robot, brain, sparkles,
  speech balloon, thought balloon, magic wand, microphone, speaker, muted
  speaker, signal/wifi, cloud, gear, lightning, check, cross, warning,
  battery). Unlike `AiIcons`, these are real per-pixel color -- there is no
  single tint color to set.
- **Use it when**: you want the icon to look like the actual multi-color
  emoji (this is what most AI chat UIs use for status/personality icons).
- **How to use**:
  ```cpp
  #include "../icons/AiEmoji.h"
  lv_obj_t *img = lv_image_create(parent);
  lv_image_set_src(img, AiEmoji::kRobot);
  ```
- **Format**: 32x32 PNG rasterized from the Twemoji SVGs, converted to LVGL
  image descriptors (`lv_image_dsc_t`) in `RGB565A8` format (16-bit color +
  8-bit alpha plane) -- smaller than full ARGB8888 with no visible quality
  loss on this display, and (deliberately) **uncompressed**, for the same
  reason `AiIcons` requires `--no-compress` above: this project's LVGL
  version doesn't reliably support compressed image data on this code path,
  so stick to `NONE` compression for any new icon added here.
- **Regenerating/extending**:
  1. Find the emoji's codepoint (e.g. https://unicode.org/emoji or just the
     hex codepoint of the character) and locate its SVG at
     `https://github.com/jdecked/twemoji/tree/main/assets/svg/<hex>.svg`
     (filename is the lowercase hex codepoint, no leading zeros, dash-joined
     for multi-codepoint sequences).
  2. Rasterize to a 32x32 PNG. This project used
     [resvg](https://github.com/RazrFalcon/resvg) (static binary, no
     dependencies): `resvg -w 32 -h 32 <hex>.svg <name>.png`.
  3. Convert to an LVGL C image with LVGL's own converter script
     (`scripts/LVGLImage.py` in the lvgl library source -- requires `pip
     install pypng lz4`):
     ```
     python LVGLImage.py --ofmt C --cf RGB565A8 --compress NONE \
       -o <output-dir> --name emoji_<name> <name>.png
     ```
  4. Fix its include block: replace the generated
     `#if defined(LV_LVGL_H_INCLUDE_SIMPLE) ... #endif` chain with a plain
     `#include <lvgl.h>`, same reasoning as the font file above.
  5. Copy the resulting `emoji_<name>.c` into `src/ui/icons/`, add an
     `extern const lv_image_dsc_t emoji_<name>;` and an `AiEmoji::k<Name>`
     alias to `AiEmoji.h`.

## Why two separate mechanisms

LVGL fonts (`AiIcons`) and LVGL images (`AiEmoji`) are different rendering
paths in LVGL: fonts are drawn through the text/label system and can only
carry one alpha-blended tint color per glyph, while images are drawn
through the image system and can carry full per-pixel color. There's no
single API that does both -- a "color font" isn't a thing LVGL 9's default
renderer supports here. Pick whichever fits: a status icon that should
match surrounding text color wants `AiIcons`; a recognizable full-color
emoji wants `AiEmoji`.
