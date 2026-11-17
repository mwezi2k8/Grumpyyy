/*
  Fuck_Off_Text - big shaking "FUCK OFF" text
  Grumpyyy: XIAO ESP32-C3 + 128x64 SSD1306 OLED
  Libraries: Adafruit SSD1306, Adafruit GFX
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Seeed XIAO ESP32-C3: SDA = D4 (GPIO6), SCL = D5 (GPIO7)
#define SDA_PIN 6
#define SCL_PIN 7
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

void startDisplay() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    for (;;) delay(1000);   // OLED not found: check wiring / address (try 0x3D)
  }
  randomSeed(micros());
}

const char *MESSAGE = "FUCK OFF";   // about 10 characters fit at text size 2

void setup() {
  startDisplay();
  display.setTextSize(2);           // 12x16 px per character
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(MESSAGE, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_W - w) / 2 + random(-1, 2);   // slight shake
  int y = (SCREEN_H - h) / 2 + random(-1, 2);
  display.clearDisplay();
  display.setCursor(x, y);
  display.print(MESSAGE);
  display.display();
  delay(40);
}
