#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Joystick
#define JOY_X 34
#define JOY_Y 35   // not used yet, but wired
#define JOY_SW 32  // pressed = LOW (with INPUT_PULLUP)

// Optional buttons (keep if you want them still)
#define USE_BUTTONS 0
#if USE_BUTTONS
  #define BTN_LEFT  25
  #define BTN_RIGHT 26
#endif

// --- Simple 1-bit sprites (PROGMEM) ---
// 12x8 "ship"
#define SHIP_W 12
#define SHIP_H 8
const uint8_t PROGMEM ship_bmp[] = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b11111111,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000,
  0b00011000,
  0b00111100,
  0b01100110,
  0b11000011
};

// 8x8 "asteroid"
#define AST_W 8
#define AST_H 8
const uint8_t PROGMEM ast_bmp[] = {
  0b00111100,
  0b01111110,
  0b11111111,
  0b11011011,
  0b11111111,
  0b11100111,
  0b01111110,
  0b00111100
};

// Game state
int playerX = 58;
const int playerY = 54;

int blockX = 0;
int blockY = 0;

int score = 0;
bool gameOver = false;

// Timing
unsigned long lastFrame = 0;
const unsigned long frameMs = 35;

// Joystick calibration
int joyCenter = 2048;   // will be measured at startup
const int deadzone = 180; // tweak if drift

// Utility
bool rectOverlap(int ax,int ay,int aw,int ah,int bx,int by,int bw,int bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

int readJoyX() {
  // ESP32 analogRead returns 0..4095
  return analogRead(JOY_X);
}

void seedRng() {
  randomSeed(esp_random());
}

void resetGame() {
  playerX = (SCREEN_WIDTH - SHIP_W) / 2;
  blockX = random(0, SCREEN_WIDTH - AST_W);
  blockY = -AST_H;
  score = 0;
  gameOver = false;
}

void calibrateJoystick() {
  // average a few reads for center
  long sum = 0;
  for (int i = 0; i < 30; i++) {
    sum += readJoyX();
    delay(5);
  }
  joyCenter = sum / 30;
}

void drawStars() {
  // cheap "starfield": deterministic stars based on score
  // (no need to store lots of points)
  for (int i = 0; i < 18; i++) {
    int x = (i * 23 + score * 7) % SCREEN_WIDTH;
    int y = (i * 11 + score * 3) % SCREEN_HEIGHT;
    display.drawPixel(x, y, SSD1306_WHITE);
  }
}

void drawGame() {
  display.clearDisplay();

  drawStars();

  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.print(score);

  // Player sprite
  display.drawBitmap(playerX, playerY, ship_bmp, SHIP_W, SHIP_H, SSD1306_WHITE);

  // Asteroid sprite
  display.drawBitmap(blockX, blockY, ast_bmp, AST_W, AST_H, SSD1306_WHITE);

  display.display();
}

void drawGameOver() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(22, 16);
  display.println("GAME OVER");

  display.setCursor(20, 32);
  display.print("Score: ");
  display.println(score);

  display.setCursor(6, 50);
  display.println("Press joystick to retry");

  display.display();
}

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (true) delay(100);
  }

  pinMode(JOY_SW, INPUT_PULLUP);

#if USE_BUTTONS
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
#endif

  seedRng();
  calibrateJoystick();
  resetGame();
}

void loop() {
  if (millis() - lastFrame < frameMs) return;
  lastFrame = millis();

  bool swPressed = (digitalRead(JOY_SW) == LOW);

  if (gameOver) {
    drawGameOver();
    if (swPressed) {
      // simple debounce
      delay(150);
      resetGame();
    }
    return;
  }

  // --- Input: joystick X controls movement ---
  int x = readJoyX();
  int dx = 0;

  if (x > joyCenter + deadzone) dx = +3;
  else if (x < joyCenter - deadzone) dx = -3;

#if USE_BUTTONS
  // Optional: buttons override/add
  if (digitalRead(BTN_LEFT) == LOW)  dx = -3;
  if (digitalRead(BTN_RIGHT) == LOW) dx = +3;
#endif

  playerX += dx;
  if (playerX < 0) playerX = 0;
  if (playerX > SCREEN_WIDTH - SHIP_W) playerX = SCREEN_WIDTH - SHIP_W;

  // --- Update asteroid ---
  blockY += 2 + (score / 8);

  if (blockY > SCREEN_HEIGHT) {
    blockY = -AST_H;
    blockX = random(0, SCREEN_WIDTH - AST_W);
    score++;
  }

  // --- Collision ---
  if (rectOverlap(playerX, playerY, SHIP_W, SHIP_H, blockX, blockY, AST_W, AST_H)) {
    gameOver = true;
  }

  drawGame();
}
