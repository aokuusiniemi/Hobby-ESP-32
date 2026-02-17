#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Buttons (to GND, using internal pullups)
#define BTN_LEFT  25
#define BTN_RIGHT 26

// Game state
int playerX = 60;               // 0..127
const int playerY = 56;         // near bottom
const int playerW = 10;
const int playerH = 6;

int blockX = 0;
int blockY = 0;
int blockW = 8;
int blockH = 8;

int score = 0;
bool gameOver = false;

// Timing
unsigned long lastFrame = 0;
const unsigned long frameMs = 35; // ~28 FPS

// Simple RNG seed
void seedRng() {
  randomSeed(esp_random());
}

void resetGame() {
  playerX = 60;
  blockX = random(0, SCREEN_WIDTH - blockW);
  blockY = 0;
  score = 0;
  gameOver = false;
}

bool rectOverlap(int ax,int ay,int aw,int ah,int bx,int by,int bw,int bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (true) delay(100);
  }

  seedRng();
  resetGame();
}

void drawGame() {
  display.clearDisplay();

  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

  // Player
  display.fillRect(playerX, playerY, playerW, playerH, SSD1306_WHITE);

  // Falling block
  display.fillRect(blockX, blockY, blockW, blockH, SSD1306_WHITE);

  display.display();
}

void drawGameOver() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(20, 18);
  display.println("GAME OVER");

  display.setCursor(18, 32);
  display.print("Score: ");
  display.println(score);

  display.setCursor(6, 48);
  display.println("Hold both to restart");

  display.display();
}

void loop() {
  if (millis() - lastFrame < frameMs) return;
  lastFrame = millis();

  // Read buttons (pressed = LOW)
  bool leftPressed  = (digitalRead(BTN_LEFT) == LOW);
  bool rightPressed = (digitalRead(BTN_RIGHT) == LOW);

  if (gameOver) {
    drawGameOver();
    if (leftPressed && rightPressed) {
      resetGame();
    }
    return;
  }

  // Move player
  if (leftPressed)  playerX -= 3;
  if (rightPressed) playerX += 3;
  if (playerX < 0) playerX = 0;
  if (playerX > SCREEN_WIDTH - playerW) playerX = SCREEN_WIDTH - playerW;

  // Move block
  blockY += 3 + (score / 10); // speeds up over time

  // If block passed bottom, respawn + score
  if (blockY > SCREEN_HEIGHT) {
    blockY = -blockH;
    blockX = random(0, SCREEN_WIDTH - blockW);
    score++;
  }

  // Collision
  if (rectOverlap(playerX, playerY, playerW, playerH, blockX, blockY, blockW, blockH)) {
    gameOver = true;
  }

  drawGame();
}
