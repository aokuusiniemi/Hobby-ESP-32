#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// Most I2C 0.96" modules don't use a separate reset pin
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES 16  // a few more looks nicer on 128x64

// "Indexes" into the icons array
#define XPOS   0
#define YPOS   1
#define DELTAY 2
#define DELTAX 3

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);       // SDA=21, SCL=22 for ESP32 DevKit V1
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed");
    while (1) delay(10);
  }

  randomSeed(esp_random()); // better randomness on ESP32

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Snow demo (dots + drift)");
  display.display();
  delay(800);
}

void loop() {
  testanimate(); // runs forever
}

// A simple particle "snow" animation using dots + horizontal drift
void testanimate() {
  int8_t f;
  // Each flake: x, y, dy, dx
  int16_t icons[NUMFLAKES][4];

  // Initialize flakes
  for (f = 0; f < NUMFLAKES; f++) {
    icons[f][XPOS]   = random(0, display.width());
    icons[f][YPOS]   = random(-display.height(), 0);
    icons[f][DELTAY] = random(1, 4);    // fall speed (1..3)
    icons[f][DELTAX] = random(-1, 2);   // drift (-1,0,+1)
  }

  for (;;) {
    display.clearDisplay();

    // Draw flakes (tiny dots)
    for (f = 0; f < NUMFLAKES; f++) {
      // radius 1 looks like snow on 128x64; change to 0 for single-pixel flakes
      display.fillCircle(icons[f][XPOS], icons[f][YPOS], 1, SSD1306_WHITE);
    }

    display.display();
    delay(50);

    // Update positions
    for (f = 0; f < NUMFLAKES; f++) {
      icons[f][YPOS] += icons[f][DELTAY];
      icons[f][XPOS] += icons[f][DELTAX];

      // Wrap X around screen edges
      if (icons[f][XPOS] < 0) icons[f][XPOS] = display.width() - 1;
      if (icons[f][XPOS] >= display.width()) icons[f][XPOS] = 0;

      // If flake falls off bottom, respawn at top with new random properties
      if (icons[f][YPOS] >= display.height()) {
        icons[f][XPOS]   = random(0, display.width());
        icons[f][YPOS]   = random(-16, 0);
        icons[f][DELTAY] = random(1, 4);
        icons[f][DELTAX] = random(-1, 2);
      }
    }
  }
}