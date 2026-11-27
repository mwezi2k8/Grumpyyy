/*
  Normal_Eyes - rounded square eyes that blink at random
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

// ---------- blink timing ----------
const uint32_t BLINK_MS = 180;
uint32_t nextBlinkAt = 0, blinkStart = 0;
bool blinking = false;

// returns how open the eyes are: 1.0 = open, 0.0 = shut
float blinkAmount(uint32_t now) {
  if (!blinking && now >= nextBlinkAt) { blinking = true; blinkStart = now; }
  if (!blinking) return 1.0;
  float t = (float)(now - blinkStart) / BLINK_MS;
  if (t >= 1.0) { blinking = false; nextBlinkAt = now + random(2000, 5000); return 1.0; }
  return 1.0 - (t < 0.5 ? t * 2.0 : (1.0 - t) * 2.0);
}

// ---------- eye shape (same as the main keychain sketch) ----------
const int EYE_W = 34, EYE_H = 40, EYE_R = 10, EYE_Y = 32;
const int LEFT_X = 36, RIGHT_X = 92;

void drawEye(int cx, int cy, float open, bool isLeft, bool angry) {
  int h = max(4, (int)(EYE_H * open));
  int top = cy - h / 2;
  int left = cx - EYE_W / 2;
  display.fillRoundRect(left, top, EYE_W, h, min(EYE_R, h / 2), SSD1306_WHITE);
  if (angry) {
    // black triangle over the top, slanting down toward the nose = angry brow
    int drop = (int)(h * 0.6);
    int x0 = left - 1, x1 = left + EYE_W + 1;
    if (isLeft) display.fillTriangle(x0, top - 1, x1, top - 1, x1, top + drop, SSD1306_BLACK);
    else        display.fillTriangle(x0, top - 1, x1, top - 1, x0, top + drop, SSD1306_BLACK);
  }
}

void setup() {
  startDisplay();
  nextBlinkAt = millis() + random(1000, 3000);
}

void loop() {
  float open = blinkAmount(millis());
  display.clearDisplay();
  drawEye(LEFT_X,  EYE_Y, open, true,  false);
  drawEye(RIGHT_X, EYE_Y, open, false, false);
  display.display();
  delay(20);
}
