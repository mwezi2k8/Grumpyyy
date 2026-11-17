/*
  Round_Eyes - big round eyes with pupils that look around, plus blinking
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

// ---------- eye shape ----------
const int EYE_R = 22, PUPIL_R = 9, EYE_Y = 32;
const int LEFT_X = 36, RIGHT_X = 92;
const int LOOK_MAX = EYE_R - PUPIL_R - 3;   // how far the pupil can move

float lookX = 0, lookY = 0;                 // current pupil offset
float targetX = 0, targetY = 0;
uint32_t nextLookAt = 0;

void drawRoundEye(int cx, int cy, float open) {
  display.fillCircle(cx, cy, EYE_R, SSD1306_WHITE);
  int px = cx + (int)lookX, py = cy + (int)lookY;
  display.fillCircle(px, py, PUPIL_R, SSD1306_BLACK);           // pupil
  display.fillCircle(px - 3, py - 3, 2, SSD1306_WHITE);         // shine
  // eyelids: black bars closing from top and bottom
  int lid = (int)((1.0 - open) * (EYE_R + 1));
  if (lid > 0) {
    display.fillRect(cx - EYE_R - 1, cy - EYE_R - 1, EYE_R * 2 + 3, lid, SSD1306_BLACK);
    display.fillRect(cx - EYE_R - 1, cy + EYE_R + 2 - lid, EYE_R * 2 + 3, lid, SSD1306_BLACK);
  }
}

void setup() {
  startDisplay();
  nextBlinkAt = millis() + random(1500, 4000);
}

void loop() {
  uint32_t now = millis();

  // pick a new place to look every 1-3 s (often straight ahead)
  if (now >= nextLookAt) {
    if (random(3) == 0) { targetX = 0; targetY = 0; }
    else { targetX = random(-LOOK_MAX, LOOK_MAX + 1); targetY = random(-LOOK_MAX / 2, LOOK_MAX / 2 + 1); }
    nextLookAt = now + random(1000, 3000);
  }
  lookX += (targetX - lookX) * 0.2;   // smooth movement
  lookY += (targetY - lookY) * 0.2;

  float open = blinkAmount(now);
  display.clearDisplay();
  drawRoundEye(LEFT_X,  EYE_Y, open);
  drawRoundEye(RIGHT_X, EYE_Y, open);
  display.display();
  delay(20);
}
