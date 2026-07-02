# Color emoji icon set for Socket's LVGL UI

Date: 2026-07-01

## Context

`src/ui/fonts/AiIcons.h` + `lv_font_ai_icons_24.c` already give Socket a
20-glyph monochrome icon set (real FiraCode Nerd Font Mono codepoints,
tinted via LVGL's font-rendering path) displayed as a test row on
`OfflineScreen`. The user wants a second, **full-color** icon set — the
kind of emoji AI assistants commonly use (robot, brain, sparkles, etc.) —
since LVGL fonts can only render single-tint alpha bitmaps, not per-pixel
color. Full color requires a different LVGL primitive: image widgets
(`lv_img_dsc_t` + `lv_img_create()`) instead of font glyphs.

## Decisions

- **Source**: Twemoji (Twitter's open-source emoji set, CC-BY 4.0).
  Flat/clean style, matches what most AI chat UIs use, easy SVG source,
  permissive license compatible with this project's existing use of
  Apache-2.0/MIT third-party assets (Espressif BSP drivers, Nerd Fonts).
- **Icon set** (17 icons, mirrors the existing Nerd Font list so the two
  sets are directly comparable): robot, brain, sparkles, speech balloon,
  thought balloon, magic wand, microphone, speaker, muted speaker,
  signal/wifi, cloud, gear, lightning, check, cross, warning, battery.
- **Pipeline**: Twemoji SVG -> rasterize to 32x32 PNG (bigger than the
  24px mono font since color detail needs more pixels than a tinted
  alpha mask) -> convert to an LVGL image C array via LVGL's image
  converter, using `RGB565A8` format (16-bit color + 8-bit alpha plane).
  This is much smaller than full ARGB8888 with no visible quality loss
  on this display, and is a standard/well-supported LVGL image format
  (unlike the compressed-font path that caused the crash-loop bug fixed
  earlier this session).
- **Rendering**: each icon becomes its own `lv_img_dsc_t` displayed via
  `lv_img_create()` + `lv_img_set_src()`, not a label+font. This is a
  different LVGL widget type than the existing `AiIcons` usage.
- **Placement**: add a second, separate test row on `OfflineScreen`,
  below the existing monochrome row, so both can be visually compared
  before deciding where either actually lives permanently in the UI.
  Neither icon set has a permanent home yet — that's an explicitly
  deferred decision from the prior session.
- **File layout**: new `src/ui/icons/` directory (separate from
  `src/ui/fonts/` since these are images, not a font) — one `.c`/`.h`
  pair per icon, following the same per-symbol-constant pattern as
  `AiIcons.h` (e.g. `AiEmoji::kRobot` as an `extern const lv_img_dsc_t *`
  or similar).
- **Documentation**: create `docs/ICONS.md` as the single reference for
  both icon sets — what each contains, how to use each from application
  code (label+font vs image widget), licensing/attribution for both
  Nerd Fonts and Twemoji, and how to regenerate/extend either set later
  (the `lv_font_conv` command for the mono set, the SVG->PNG->LVGL-image
  pipeline for the color set). This is so a future agent or developer
  picking up this repo doesn't have to reverse-engineer either pipeline
  from scratch.

## Out of scope

- Deciding the permanent UI location for either icon set (status bar,
  conversation screen, etc.) — still deferred.
- Removing the existing monochrome test row — left in place for
  comparison per this design.
- Any additional emoji beyond the 17 listed above.
