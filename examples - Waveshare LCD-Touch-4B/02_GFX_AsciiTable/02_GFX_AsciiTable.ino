#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include <Wire.h>

Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
  7,
  0,
  2,
  1,
  &Wire,
  0x20);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  17 /* DE */, 3 /* VSYNC */, 46 /* HSYNC */, 9 /* PCLK */,
  10 /* B0 */, 11 /* B1 */, 12 /* B2 */, 13 /* B3 */, 14 /* B4 */,
  21 /* G0 */, 8 /* G1 */, 18 /* G2 */, 45 /* G3 */, 38 /* G4 */, 39 /* G5 */,
  40 /* R0 */, 41 /* R1 */, 42 /* R2 */, 2 /* R3 */, 1 /* R4 */,
  1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
  1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
  expander, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));


void setup(void) {
  Serial.begin(115200);
  Serial.println("Arduino_GFX AsciiTable example");
  Wire.begin(47, 48);

  int charWidth = 8;
  int charHeight = 10;

  int numCols = 480 / charWidth;
  int numRows = 480 / charHeight;

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  expander->pinMode(5, OUTPUT);
  expander->pinMode(6, OUTPUT);
  expander->digitalWrite(6, LOW);
  delay(200);
  expander->digitalWrite(5, LOW);
  delay(200);
  expander->digitalWrite(5, HIGH);
  delay(200);

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

  gfx->setTextColor(GREEN);
  for (int x = 0; x < numCols; x++) {
    if (x % 4 == 0) {
      gfx->setCursor(10 + x * charWidth, 2);
      gfx->print(x, 16);
    }
  }


  gfx->setTextColor(BLUE);
  for (int y = 0; y < numCols; y++) {
    gfx->setCursor(2, 12 + y * charHeight);
    gfx->print(y, 16);
  }

  char c = 0;
  for (int y = 0; y < numRows; y++) {
    for (int x = 0; x < numCols; x++) {
      gfx->drawChar(10 + x * charWidth, 12 + y * charHeight, c++, WHITE, BLACK);
    }
  }

  delay(5000);
}

void loop() {
}
