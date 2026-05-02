/**
 * Socket — 01_HelloWorld
 *
 * First test sketch for the Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
 * Initializes the MIPI DSI display and prints "Hello World!" to the screen.
 *
 * Board: ESP32P4 Dev Module
 * PSRAM: Enabled (required — framebuffer lives in PSRAM)
 * Flash: QIO 80MHz, 16MB
 * USB CDC On Boot: Disabled (uses UART port)
 *
 * Required libraries:
 *   - GFX_Library_for_Arduino  (from lib/vendored/)
 *   - displays                 (from lib/vendored/)
 *   - lvgl v9.3.0              (install via Library Manager)
 */

#ifndef BOARD_HAS_PSRAM
#error "This program requires PSRAM enabled. Enable it in Tools > PSRAM in Arduino IDE."
#endif

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "displays_config.h"

/* ============================================================
 *  ESP32-P4-WIFI6-Touch-LCD-4B Custom Display Configuration
 *
 *  Display: 4" round IPS, 720x720
 *  Controller: ST7703 via MIPI-DSI (2 lanes)
 *  DSI PHY: LDO channel 3 @ 2500mV
 *
 *  Timing:
 *    Horizontal: 720 active, 20 hsync, 80 hbp, 80 hfp
 *    Vertical:   720 active,  4 vsync, 12 vbp, 30 vfp
 *  DPI clock: 46 MHz
 *  Lane bit rate: 750 Mbps
 * ============================================================ */

#define P4_4B_HSYNC_PULSE    20
#define P4_4B_HSYNC_BP       80
#define P4_4B_HSYNC_FP       80
#define P4_4B_VSYNC_PULSE    4
#define P4_4B_VSYNC_BP       12
#define P4_4B_VSYNC_FP       30
#define P4_4B_WIDTH          720
#define P4_4B_HEIGHT         720
#define P4_4B_DPI_CLOCK      46000000UL
#define P4_4B_LANE_BIT_RATE  750  // Mbps

/* Minimal init sequence — Sleep Out then Display On.
 * A full ST7703 init sequence with gamma/PVcom/GIP tuning
 * can be added later from the panel datasheet. */
static const lcd_init_cmd_t p4_4b_init[] = {
    {0x11, (uint8_t[]){0x00}, 0, 120},  // Sleep Out + 120ms delay
    {0x29, (uint8_t[]){0x00}, 0, 20},   // Display On + 20ms delay
};

/* ============================================================ */

Arduino_ESP32DSIPanel *dsipanel;
Arduino_DSI_Display *gfx;

void setup() {
  Serial.begin(115200);
  Serial.println("Socket HelloWorld starting...");

  /* --- Backlight: GPIO 26, inverted (LOW = ON) --- */
  pinMode(26, OUTPUT);
  digitalWrite(26, LOW);

  /* --- I2C init for touch/backlight control --- */
  DEV_I2C_Port port = DEV_I2C_Init();
  display_init(port);
  set_display_backlight(port, 255);

  /* --- MIPI DSI panel bus --- */
  dsipanel = new Arduino_ESP32DSIPanel(
      P4_4B_HSYNC_PULSE, P4_4B_HSYNC_BP, P4_4B_HSYNC_FP,
      P4_4B_VSYNC_PULSE, P4_4B_VSYNC_BP, P4_4B_VSYNC_FP,
      P4_4B_DPI_CLOCK, P4_4B_LANE_BIT_RATE);

  /* --- Display driver (ST7703 via DSI) --- */
  gfx = new Arduino_DSI_Display(
      P4_4B_WIDTH, P4_4B_HEIGHT,
      dsipanel,
      0,              // rotation
      true,           // auto_flush
      -1,             // RST pin (handled by DSI panel)
      p4_4b_init, sizeof(p4_4b_init) / sizeof(lcd_init_cmd_t));

  delay(200);

  if (!gfx->begin()) {
    Serial.println("gfx->begin() FAILED!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("Display initialized OK");

  /* --- Show Hello World --- */
  gfx->fillScreen(RGB565_BLACK);
  gfx->setCursor(90, 330);
  gfx->setTextColor(RGB565_RED);
  gfx->setTextSize(4);
  gfx->println("Hello World!");

  Serial.println("Hello World displayed!");
}

void loop() {
  /* Animate: cycle text color every second */
  static uint32_t lastToggle = 0;
  static uint8_t hue = 0;

  if (millis() - lastToggle >= 1000) {
    lastToggle = millis();
    hue = (hue + 32) & 0x1F;

    /* Generate a 16-bit RGB565 color from hue */
    uint16_t color = (hue << 11) | ((hue << 6) & 0x07E0) | (hue & 0x1F);

    gfx->fillRect(90, 330, 540, 50, RGB565_BLACK);
    gfx->setCursor(90, 330);
    gfx->setTextSize(4);
    gfx->setTextColor(color);
    gfx->println("Hello World!");
  }

  delay(10);
}
