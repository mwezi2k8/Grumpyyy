/*
  Grumpyyy
  Seeed XIAO ESP32-C3 + MPU6050 + 0.96" SSD1306 OLED (128x64, I2C), powered over USB

  - Normal: eyes blink at random intervals
  - Shake it for 2 seconds: angry eyes
  - While angry, every shake restarts the 5 s timer (stays angry while you shake)
  - 5 seconds after the last shake it shows "FUCK OFF" for 2 s, then goes back to normal
  - Shake for 2 s again any time to make it angry again

  Libraries (Library Manager): Adafruit SSD1306, Adafruit GFX
  (MPU6050 is read directly over I2C, no library needed)

  Wiring (OLED and MPU6050 share the same two I2C wires):
    3V3 -> OLED VCC, MPU6050 VCC
    GND -> OLED GND, MPU6050 GND
    D4 (GPIO6, SDA) -> OLED SDA, MPU6050 SDA
    D5 (GPIO7, SCL) -> OLED SCL, MPU6050 SCL
  MPU6050 AD0 -> GND (address 0x68). OLED address is usually 0x3C.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- pins: Seeed XIAO ESP32-C3 ----------
#define SDA_PIN 6   // D4
#define SCL_PIN 7   // D5

// ---------- display ----------
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ---------- MPU6050 ----------
#define MPU_ADDR 0x68

// ---------- tuning ----------
const float    SHAKE_THRESHOLD_G = 0.6;   // deviation from 1g that counts as shaking (lower = more sensitive)
const uint32_t SHAKE_NEEDED_MS   = 2000;  // keep shaking this long to get angry
const uint32_t SHAKE_GAP_MS      = 400;   // pause longer than this resets the shake timer
const uint32_t ANGRY_HOLD_MS     = 5000;  // angry duration, then back to normal

// ---------- eye geometry ----------
const int EYE_W   = 34;
const int EYE_H   = 40;
const int EYE_R   = 10;
const int EYE_Y   = 32;
const int LEFT_X  = 36;
const int RIGHT_X = 92;

// ---------- state ----------
enum Mood { NORMAL, ANGRY, SAYING };
Mood mood = NORMAL;
uint32_t angryStart = 0;
uint32_t sayStart = 0;
const uint32_t SAY_MS = 2000;           // how long the text stays on screen
const char *ANGRY_TEXT = "FUCK OFF";    // shown when the angry phase ends

uint32_t shakeStart = 0;      // 0 = not currently counting
uint32_t lastShakeSeen = 0;

// blink
uint32_t nextBlinkAt = 0;
uint32_t blinkStart = 0;
bool blinking = false;
const uint32_t BLINK_MS = 180;

float lid = 0.0;              // 0 = no angry lid, 1 = full angry lid (eased)

// ---------- MPU helpers ----------
void mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);   // wake up
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); Wire.write(0x08);   // accel range +-4g (8192 LSB/g)
  Wire.endTransmission();
}

bool readAccelG(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6) != 6) return false;
  int16_t x = (Wire.read() << 8) | Wire.read();
  int16_t y = (Wire.read() << 8) | Wire.read();
  int16_t z = (Wire.read() << 8) | Wire.read();
  ax = x / 8192.0; ay = y / 8192.0; az = z / 8192.0;
  return true;
}

// ---------- shake logic ----------
// returns true once the user has shaken for SHAKE_NEEDED_MS
bool updateShake(uint32_t now) {
  float ax, ay, az;
  if (!readAccelG(ax, ay, az)) return false;
  float mag = sqrt(ax * ax + ay * ay + az * az);
  bool shaking = fabs(mag - 1.0) > SHAKE_THRESHOLD_G;

  if (shaking) {
    lastShakeSeen = now;
    if (shakeStart == 0) shakeStart = now;
  } else if (shakeStart != 0 && now - lastShakeSeen > SHAKE_GAP_MS) {
    shakeStart = 0;   // stopped shaking, start over
  }

  return shakeStart != 0 && (now - shakeStart >= SHAKE_NEEDED_MS);
}

// ---------- drawing ----------
void drawEye(int cx, int cy, float open, bool isLeft, float lidAmt, int dx, int dy) {
  int h = max(4, (int)(EYE_H * open));
  int top = cy - h / 2 + dy;
  int left = cx - EYE_W / 2 + dx;
  int r = min(EYE_R, h / 2);
  display.fillRoundRect(left, top, EYE_W, h, r, SSD1306_WHITE);

  if (lidAmt > 0.01) {
    // black triangle over the top, slanting down toward the nose = angry brow
    int drop = (int)(h * 0.6 * lidAmt);
    int x0 = left - 1, x1 = left + EYE_W + 1;
    if (isLeft) {
      display.fillTriangle(x0, top - 1, x1, top - 1, x1, top + drop, SSD1306_BLACK);
    } else {
      display.fillTriangle(x0, top - 1, x1, top - 1, x0, top + drop, SSD1306_BLACK);
    }
  }
}

void drawFace(float open, float lidAmt, int dx, int dy) {
  display.clearDisplay();
  drawEye(LEFT_X,  EYE_Y, open, true,  lidAmt, dx, dy);
  drawEye(RIGHT_X, EYE_Y, open, false, lidAmt, dx, dy);
  display.display();
}

void drawText(const char *msg) {
  display.clearDisplay();
  display.setTextSize(2);                 // 12x16 px per character
  display.setTextColor(SSD1306_WHITE);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_W - w) / 2 + random(-1, 2);   // slight shake
  int y = (SCREEN_H - h) / 2 + random(-1, 2);
  display.setCursor(x, y);
  display.print(msg);
  display.display();
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    for (;;) delay(1000);   // OLED not found: check wiring / address
  }
  mpuInit();
  randomSeed(micros());
  nextBlinkAt = millis() + random(2000, 5000);
}

void loop() {
  uint32_t now = millis();

  // ----- mood logic -----
  bool shakeDone = updateShake(now);
  if (mood == NORMAL && shakeDone) {
    mood = ANGRY;
    angryStart = now;
    shakeStart = 0;
  } else if (mood == ANGRY) {
    // any shake while angry restarts the 5 s timer, so it stays angry
    // for as long as you keep shaking, and calms 5 s after the last shake
    if (lastShakeSeen != 0 && now - lastShakeSeen < SHAKE_GAP_MS) {
      angryStart = now;
    }
    if (now - angryStart >= ANGRY_HOLD_MS) {
      mood = SAYING;            // angry is over: show the text
      sayStart = now;
      shakeStart = 0;
    }
  } else if (mood == SAYING) {
    if (lastShakeSeen != 0 && now - lastShakeSeen < SHAKE_GAP_MS) {
      mood = ANGRY;             // shaken again while talking: straight back to angry
      angryStart = now;
    } else if (now - sayStart >= SAY_MS) {
      mood = NORMAL;
      shakeStart = 0;
    }
  }

  if (mood == SAYING) {
    drawText(ANGRY_TEXT);
    delay(20);
    return;
  }

  // ease the angry lid in and out
  float target = (mood == ANGRY) ? 1.0 : 0.0;
  lid += (target - lid) * 0.25;

  // ----- blink logic -----
  float open = 1.0;
  if (!blinking && now >= nextBlinkAt) {
    blinking = true;
    blinkStart = now;
  }
  if (blinking) {
    float t = (float)(now - blinkStart) / BLINK_MS;   // 0..1
    if (t >= 1.0) {
      blinking = false;
      nextBlinkAt = now + random(2000, 5000);
    } else {
      open = 1.0 - (t < 0.5 ? t * 2.0 : (1.0 - t) * 2.0);   // close then open
    }
  }

  // angry eyes tremble slightly
  int dx = 0, dy = 0;
  if (mood == ANGRY) {
    dx = random(-1, 2);
    dy = random(-1, 2);
  }

  drawFace(open, lid, dx, dy);
  delay(20);   // ~50 fps max
}
