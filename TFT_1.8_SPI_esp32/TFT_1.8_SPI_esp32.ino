#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ====== ESP32 Pins (VSPI) ======
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  15

#define TFT_SCLK 18
#define TFT_MOSI 23



Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


void setup() {
  Serial.begin(115200);

  // PWM backlight

      // max brightness

  // SPI init (explicit pins)
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // ---- Display init: try these one by one if mirrored/offset ----
  // Most common for 1.8" 128x160:
  tft.initR(INITR_BLACKTAB);

  // If mirrored or shifted, comment the above and try:
  // tft.initR(INITR_GREENTAB);
  // tft.initR(INITR_REDTAB);
  // tft.initR(INITR_MINI160x80); // only if you actually have 160x80

  tft.setRotation(0); // try 0,1,2,3 to fix mirror/orientation

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(10, 20);
  tft.print("ESP OK");

  tft.drawRect(0, 0, 128, 160, ST77XX_GREEN);
}

void loop() {
  // demo: pulse brightness
  // for (int b = 30; b <= 255; b += 5) { setBacklight(b); delay(10); }
  // for (int b = 255; b >= 30; b -= 5) { setBacklight(b); delay(10); }
}
