// A Taste of Memory — Sienna Berreby
// ESP32 TTGO T-Display, 240 x 135 landscape; TFT_eSPI Setup25.
// Procedurally drawn animation inspired by the tasting flashback in Ratatouille.
// Documentation comments added; original program behavior preserved.

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_system.h>
#include <math.h>

TFT_eSPI tft;
TFT_eSprite plate(&tft), frame(&tft);

const int W = 240, H = 135;
const int SPARKS = 14;
const uint32_t PLATE_MS = 6000, BITE_MS = 1200, FADE_MS = 1300, MEMORY_MS = 6500;

uint32_t cycleStart, lastFrame;
int biteX, biteY;

struct Spark {
  float angle;
  float distance;
};

Spark sparks[SPARKS];

uint16_t col(int r, int g, int b) {
  return tft.color565(r, g, b);
}

uint16_t blend(uint16_t a, uint16_t b, int amount) {
  amount = constrain(amount, 0, 255);
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  int r = ar + (br - ar) * amount / 255;
  int g = ag + (bg - ag) * amount / 255;
  int bl = ab + (bb - ab) * amount / 255;
  return (r << 11) | (g << 5) | bl;
}

void vegetable(int x, int y, int s, int type) {
  uint16_t outside, inside;

  if (type == 0) { // tomato
    outside = col(145, 48, 35);
    inside = col(215, 85, 55);
  } else if (type == 1) { // zucchini
    outside = col(55, 95, 45);
    inside = col(155, 175, 90);
  } else if (type == 2) { // eggplant
    outside = col(70, 45, 75);
    inside = col(195, 170, 130);
  } else { // squash
    outside = col(180, 125, 45);
    inside = col(225, 185, 85);
  }

  plate.fillEllipse(x + 1, y + 1, s, s - 2, col(75, 45, 30)); // shadow
  plate.fillEllipse(x, y, s, s - 2, outside); // skin
  plate.fillEllipse(x, y, s - 2, s - 4, inside); // center
}

// Generate one plate per cycle; keep it stable while the bite animates.
// random(min, max) excludes max: the outer ring contains 17–20 slices.
void makePlate() {
  plate.fillSprite(col(32, 28, 27)); // background

  plate.fillCircle(121, 69, 50, col(18, 18, 18)); // shadow
  plate.fillCircle(120, 67, 48, col(235, 220, 190)); // plate
  plate.drawCircle(120, 67, 43, col(180, 160, 125)); // rim
  plate.fillCircle(120, 67, 36, col(130, 60, 37)); // sauce

  int count = random(17, 21);

  for (int i = 0; i < count; i++) {
    float a = TWO_PI * i / count;
    int r = 27 + random(-2, 3);
    int x = 120 + cos(a) * r;
    int y = 67 + sin(a) * r;
    vegetable(x, y, random(6, 9), i % 4); // vegetables
  }

  for (int i = 0; i < 9; i++) {
    float a = TWO_PI * i / 9.0;
    int x = 120 + cos(a) * 13;
    int y = 67 + sin(a) * 13;
    vegetable(x, y, random(5, 7), (i + 1) % 4); // vegetables
  }

  for (int i = 0; i < 5; i++)
    plate.fillEllipse(random(98, 143), random(47, 88), 3, 1, col(60, 95, 50)); // herbs

  biteX = 145 + random(-4, 5);
  biteY = 80 + random(-3, 4);
}

// Draw the memory from geometric primitives. Time-driven sine waves animate
// steam and dust; the kitchen layout itself stays constant.
void drawKitchen(float t) {
  frame.fillSprite(col(78, 53, 40)); // background

  frame.fillRect(0, 15, W, 84, col(88, 60, 44)); // wall

  frame.fillRect(33, 26, 64, 54, col(50, 35, 28)); // window
  frame.fillRect(38, 31, 54, 44, col(235, 180, 95)); // sky
  frame.fillRect(64, 31, 3, 44, col(85, 55, 35)); // frame
  frame.fillRect(38, 52, 54, 3, col(85, 55, 35)); // frame
  frame.fillCircle(50, 42, 5, col(250, 220, 145)); // sun

  frame.fillRect(0, 99, W, 8, col(145, 92, 48)); // table
  frame.fillRect(0, 107, W, 15, col(65, 42, 30)); // shadow

  int potX = 170;

  frame.fillRoundRect(potX - 15, 82, 30, 17, 4, col(150, 85, 45)); // pot
  frame.fillEllipse(potX, 82, 15, 4, col(205, 135, 70)); // rim
  frame.fillEllipse(potX, 82, 10, 2, col(75, 45, 30)); // inside

  frame.fillEllipse(112, 94, 19, 6, col(195, 135, 68)); // bread
  frame.drawLine(103, 90, 100, 95, col(230, 180, 100)); // cut
  frame.drawLine(111, 89, 108, 95, col(230, 180, 100)); // cut

  for (int w = 0; w < 3; w++) {
    for (int i = 0; i < 20; i++) {
      int y = 78 - i;
      int x = potX + (w - 1) * 6 + sin(t * 1.3 + i * 0.25 + w) * 3;
      frame.drawPixel(x, y, col(220, 200, 165)); // steam
    }
  }

  for (int i = 0; i < 8; i++) {
    int x = 28 + i * 22 + sin(t * 0.5 + i) * 3;
    int y = 34 + (i * 12 % 50) + sin(t * 0.7 + i) * 3;
    frame.drawPixel(x, y, col(225, 190, 120)); // dust
  }
}

void copyPlate() {
  frame.pushImage(0, 0, W, H, (uint16_t*)plate.getPointer());
}

// Blend every RGB565 pixel toward near-black. This is a fade through darkness,
// not a simultaneous crossfade between two images.
void darkenFrame(float amount) {
  amount = constrain(amount, 0, 1);
  uint16_t dark = col(20, 18, 18);

  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      uint16_t c = frame.readPixel(x, y);
      frame.drawPixel(x, y, blend(c, dark, amount * 255)); // fade
    }
  }
}

void drawBurst(float p) {
  for (int i = 0; i < SPARKS; i++) {
    int x = biteX + cos(sparks[i].angle) * sparks[i].distance * p;
    int y = biteY + sin(sparks[i].angle) * sparks[i].distance * p;
    frame.fillCircle(x, y, 1, col(245, 205, 125)); // sparks
  }
}

// Reseed and regenerate the plate, bite location, and 14 spark trajectories.
void newCycle() {
  randomSeed(esp_random());
  makePlate();

  for (int i = 0; i < SPARKS; i++) {
    sparks[i].angle = random(0, 628) / 100.0;
    sparks[i].distance = random(20, 70);
  }

  cycleStart = millis();
}

void setup() {
  tft.init();
  tft.setRotation(1);

  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  plate.setColorDepth(16);
  frame.setColorDepth(16);

  // Two 16-bit 240 x 135 buffers use about 129,600 bytes of pixel storage.
  // plate is the saved food image; frame is the composited output.
  plate.createSprite(W, H);
  frame.createSprite(W, H);

  newCycle();
  lastFrame = millis();
}

void loop() {
  uint32_t now = millis();

  // Request no more than 20 frames/second; rendering time can lower this rate.
  if (now - lastFrame < 50)
    return;

  lastFrame = now;
  uint32_t age = now - cycleStart;

  // Timeline: plate 6 s, bite 1.2 s, plate fade 1.3 s, kitchen fade-in
  // 1.3 s, kitchen hold 6.5 s, kitchen fade-out 1.3 s = 17.6 s.
  uint32_t biteStart = PLATE_MS;
  uint32_t plateFadeStart = biteStart + BITE_MS;
  uint32_t memoryStart = plateFadeStart + FADE_MS;
  uint32_t memoryHoldEnd = memoryStart + FADE_MS + MEMORY_MS;
  uint32_t endTime = memoryHoldEnd + FADE_MS;

  if (age < biteStart) {
    copyPlate(); // plate
  }

  else if (age < plateFadeStart) {
    float p = (age - biteStart) / (float)BITE_MS;

    copyPlate();
    frame.fillCircle(biteX, biteY, 7 * p, col(225, 210, 180)); // bite
    drawBurst(p);
  }

  else if (age < memoryStart) {
    float p = (age - plateFadeStart) / (float)FADE_MS;

    copyPlate();
    darkenFrame(p); // fade
  }

  else if (age < memoryStart + FADE_MS) {
    float p = (age - memoryStart) / (float)FADE_MS;

    drawKitchen(age / 1000.0); // kitchen
    darkenFrame(1 - p); // fade
  }

  else if (age < memoryHoldEnd) {
    drawKitchen(age / 1000.0); // memory
  }

  else if (age < endTime) {
    float p = (age - memoryHoldEnd) / (float)FADE_MS;

    drawKitchen(age / 1000.0);
    darkenFrame(p); // fade
  }

  else {
    newCycle(); // restart
    copyPlate();
  }

  frame.pushSprite(0, 0); // screen
}