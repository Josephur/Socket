/**
 * Socket — 01_HelloWorld
 *
 * First test sketch for the Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
 * Initializes the display and prints "Hello World!" to the screen.
 *
 * Board: ESP32P4 Dev Module
 * PSRAM: Enabled
 * Flash: QIO 80MHz, 16MB
 * USB CDC On Boot: Disabled
 *
 * Required libraries:
 *   - GFX_Library_for_Arduino  (display driver)
 *   - displays                 (I2C/sensor/touch drivers)
 *   - Mylibrary                (board macro definitions)
 *   - lvgl                     (optional, for future UI work)
 */

#include <Arduino.h>
#include <Mylibrary.h>
#include <displays.h>

/* Display object (ST7703 via MIPI DSI, 720x720) */
static Arduino_ESP32P4_MIPI_DSI *bus = nullptr;
static Arduino_ST7703 *gfx = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println("Socket HelloWorld starting...");

  /* Initialize backlight (GPIO 26, inverted) */
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, LOW);  // Inverted: LOW = backlight ON

  /* Create display bus: MIPI DSI, 2 lanes, 720x720 */
  bus = new Arduino_ESP32P4_MIPI_DSI(
      PIN_LCD_DSI_TE,           /* TE pin (unused on this board, pass -1) */
      PIN_LCD_DSI_RST,          /* Reset: GPIO 27 */
      2,                        /* Number of DSI lanes */
      46 * 1000000UL,           /* DPI clock: 46 MHz */
      720,                      /* Width */
      720,                      /* Height */
      {20, 80, 80},             /* H-sync, H-back, H-front porch */
      {4, 12, 30},              /* V-sync, V-back, V-front porch */
      3,                        /* MIPI DSI PHY LDO channel */
      2500                      /* LDO voltage (mV) */
  );

  /* Create display panel driver for ST7703 */
  gfx = new Arduino_ST7703(bus, PIN_LCD_DSI_RST, 0, true);

  /* Initialize the display */
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
    while (1) {
      delay(1000);
    }
  }

  /* Configure rotation for 720x720 round display */
  gfx->setRotation(0);

  /* Fill screen with black */
  gfx->fillScreen(BLACK);

  /* Draw "Hello World!" in the center */
  gfx->setCursor(180, 330);
  gfx->setTextColor(RED);
  gfx->setTextSize(4);
  gfx->println("Hello World!");

  Serial.println("Hello World displayed!");
}

void loop() {
  /* Animate: flash the text colors every second */
  static uint32_t lastToggle = 0;
  static bool useRed = true;

  if (millis() - lastToggle >= 1000) {
    lastToggle = millis();
    useRed = !useRed;

    gfx->fillRect(180, 330, 360, 40, BLACK);
    gfx->setCursor(180, 330);
    gfx->setTextSize(4);

    if (useRed) {
      gfx->setTextColor(RED);
    } else {
      gfx->setTextColor(YELLOW);
    }
    gfx->println("Hello World!");
  }

  delay(10);
}
