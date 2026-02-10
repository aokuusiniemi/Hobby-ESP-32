#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== I2C pins =====
#define SDA_PIN 21
#define SCL_PIN 22

// ===== Buttons (active LOW) =====
#define BTN_UP     32   // yellow
#define BTN_DOWN   33   // orange
#define BTN_SELECT 25   // green

// ===== Debounce + repeat =====
const uint32_t DEBOUNCE_MS = 30;
const uint32_t REPEAT_START_MS = 450;
const uint32_t REPEAT_RATE_MS  = 120;

struct Btn {
  uint8_t pin;
  bool stable;            // debounced pressed?
  bool lastReading;
  uint32_t lastChange;
  uint32_t pressedAt;
  uint32_t lastRepeat;
};

Btn bUp     {BTN_UP,     false, false, 0, 0, 0};
Btn bDown   {BTN_DOWN,   false, false, 0, 0, 0};
Btn bSelect {BTN_SELECT, false, false, 0, 0, 0};

enum EventType { EVT_NONE, EVT_PRESS, EVT_REPEAT };

EventType pollButton(Btn &b) {
  bool readingPressed = (digitalRead(b.pin) == LOW);

  if (readingPressed != b.lastReading) {
    b.lastReading = readingPressed;
    b.lastChange = millis();
  }

  if (millis() - b.lastChange > DEBOUNCE_MS) {
    if (readingPressed != b.stable) {
      b.stable = readingPressed;
      if (b.stable) {
        b.pressedAt = millis();
        b.lastRepeat = millis();
        return EVT_PRESS;
      }
    }
  }

  if (b.stable) {
    uint32_t now = millis();
    if (now - b.pressedAt >= REPEAT_START_MS && now - b.lastRepeat >= REPEAT_RATE_MS) {
      b.lastRepeat = now;
      return EVT_REPEAT;
    }
  }

  return EVT_NONE;
}

// ===== UI State Machine =====
enum ScreenState { SCREEN_MENU, SCREEN_PAGE };
ScreenState screen = SCREEN_MENU;

const char* menuItems[] = { "Snow Demo", "System Info", "I2C Scan", "Settings" };
const int MENU_COUNT = sizeof(menuItems) / sizeof(menuItems[0]);
int menuIndex = 0;
int activePage = 0;

// ===== Settings =====
bool invertDisplay = false;
int snowflakeCount = 18;

// ===== Snow particles =====
struct Flake {
  int16_t x, y;
  int8_t dy, dx;
  uint8_t r;
};
static Flake flakes[32];  // max we allow
uint32_t lastSnowFrame = 0;
const uint32_t SNOW_FRAME_MS = 50;

// ===== I2C scan cache =====
uint8_t foundAddrs[16];
int foundCount = 0;
bool scanDone = false;
int scanScroll = 0;

// ===== Helpers =====
bool selectPressed() {
  return pollButton(bSelect) == EVT_PRESS;
}

void drawHeader(const char* title) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void drawMenu() {
  display.clearDisplay();
  drawHeader("Main Menu");

  int y = 16;
  for (int i = 0; i < MENU_COUNT; i++) {
    if (i == menuIndex) {
      display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(2, y);
    display.println(menuItems[i]);
    y += 12;
  }
  display.display();
}

void enterPage(int pageIndex) {
  activePage = pageIndex;
  screen = SCREEN_PAGE;

  // Page-specific init
  if (activePage == 0) {
    // Snow init
    int n = min(snowflakeCount, (int)(sizeof(flakes) / sizeof(flakes[0])));
    for (int i = 0; i < n; i++) {
      flakes[i].x = random(0, display.width());
      flakes[i].y = random(-display.height(), 0);
      flakes[i].dy = random(1, 4);
      flakes[i].dx = random(-1, 2);
      flakes[i].r  = (random(0, 10) < 2) ? 0 : 1; // mostly radius 1, some radius 0
    }
    lastSnowFrame = 0;
  }

  if (activePage == 2) {
    // I2C Scan init
    foundCount = 0;
    scanDone = false;
    scanScroll = 0;
  }
}

void exitToMenu() {
  screen = SCREEN_MENU;
  drawMenu();
}

// ===== PAGE: Snow Demo =====
void pageSnow(EventType upEvt, EventType downEvt, EventType selEvt) {
  // SELECT exits
  if (selEvt == EVT_PRESS) {
    exitToMenu();
    return;
  }

  // Optional: change snow density while running
  if ((upEvt == EVT_PRESS || upEvt == EVT_REPEAT) && snowflakeCount < 32) snowflakeCount++;
  if ((downEvt == EVT_PRESS || downEvt == EVT_REPEAT) && snowflakeCount > 1) snowflakeCount--;

  // Frame timing
  uint32_t now = millis();
  if (now - lastSnowFrame < SNOW_FRAME_MS) return;
  lastSnowFrame = now;

  int n = min(snowflakeCount, 32);

  display.clearDisplay();
  drawHeader("Snow Demo");
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 54);
  display.print("Flakes: ");
  display.print(n);
  display.print("  SEL=Back");

  // Draw flakes
  for (int i = 0; i < n; i++) {
    if (flakes[i].r == 0) display.drawPixel(flakes[i].x, flakes[i].y, SSD1306_WHITE);
    else display.fillCircle(flakes[i].x, flakes[i].y, 1, SSD1306_WHITE);
  }

  display.display();

  // Update flakes after drawing
  for (int i = 0; i < n; i++) {
    flakes[i].y += flakes[i].dy;
    flakes[i].x += flakes[i].dx;

    if (flakes[i].x < 0) flakes[i].x = display.width() - 1;
    if (flakes[i].x >= display.width()) flakes[i].x = 0;

    if (flakes[i].y >= display.height()) {
      flakes[i].x = random(0, display.width());
      flakes[i].y = random(-16, 0);
      flakes[i].dy = random(1, 4);
      flakes[i].dx = random(-1, 2);
      flakes[i].r  = (random(0, 10) < 2) ? 0 : 1;
    }
  }
}

// ===== PAGE: System Info =====
void pageSystemInfo(EventType upEvt, EventType downEvt, EventType selEvt) {
  (void)upEvt; (void)downEvt;

  if (selEvt == EVT_PRESS) {
    exitToMenu();
    return;
  }

  // Update ~4x/sec
  static uint32_t lastDraw = 0;
  uint32_t now = millis();
  if (now - lastDraw < 250) return;
  lastDraw = now;

  uint32_t ms = millis();
  uint32_t s = ms / 1000;
  uint32_t m = s / 60; s %= 60;
  uint32_t h = m / 60; m %= 60;

  // heap API varies a bit by core version; this works in ESP32 Arduino core:
  uint32_t freeHeap = ESP.getFreeHeap();

  display.clearDisplay();
  drawHeader("System Info");

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 16);
  display.print("Uptime: ");
  display.print(h); display.print("h ");
  display.print(m); display.print("m ");
  display.print(s); display.println("s");

  display.setCursor(0, 28);
  display.print("Free heap: ");
  display.print(freeHeap / 1024);
  display.println(" KB");

  display.setCursor(0, 40);
  display.println("WiFi: (not in code)");

  display.setCursor(0, 54);
  display.println("SELECT = Back");

  display.display();
}

// ===== PAGE: I2C Scan =====
void doI2CScanOnce() {
  foundCount = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      if (foundCount < (int)sizeof(foundAddrs)) {
        foundAddrs[foundCount++] = addr;
      }
    }
    delay(2);
  }
  scanDone = true;
}

void pageI2CScan(EventType upEvt, EventType downEvt, EventType selEvt) {
  if (selEvt == EVT_PRESS) {
    exitToMenu();
    return;
  }

  if (!scanDone) {
    display.clearDisplay();
    drawHeader("I2C Scan");
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println("Scanning...");
    display.display();

    doI2CScanOnce();
  }

  if (upEvt == EVT_PRESS || upEvt == EVT_REPEAT) {
    scanScroll = max(0, scanScroll - 1);
  }
  if (downEvt == EVT_PRESS || downEvt == EVT_REPEAT) {
    scanScroll = min(max(0, foundCount - 4), scanScroll + 1);
  }

  display.clearDisplay();
  drawHeader("I2C Scan");
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 14);
  display.print("Found: ");
  display.print(foundCount);
  display.print("  SEL=Back");

  if (foundCount == 0) {
    display.setCursor(0, 30);
    display.println("No devices found.");
  } else {
    // list up to 4 at a time
    for (int i = 0; i < 4; i++) {
      int idx = scanScroll + i;
      if (idx >= foundCount) break;

      display.setCursor(0, 28 + i * 9);
      display.print("#");
      display.print(idx + 1);
      display.print(": 0x");
      if (foundAddrs[idx] < 16) display.print("0");
      display.print(foundAddrs[idx], HEX);

      if (foundAddrs[idx] == 0x3C) display.print(" (OLED)");
    }
  }

  display.display();
}

// ===== PAGE: Settings =====
void pageSettings(EventType upEvt, EventType downEvt, EventType selEvt) {
  (void)upEvt; (void)downEvt;

  // SELECT toggles invert; long-term you can add more items.
  if (selEvt == EVT_PRESS) {
    invertDisplay = !invertDisplay;
    display.invertDisplay(invertDisplay);
  }

  display.clearDisplay();
  drawHeader("Settings");
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 18);
  display.print("Invert: ");
  display.println(invertDisplay ? "ON" : "OFF");

  display.setCursor(0, 34);
  display.println("SELECT toggles");

  display.setCursor(0, 54);
  display.println("Hold UP/DN in Snow");

  display.display();
}

// ===== Arduino setup/loop =====
void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed");
    while (1) delay(10);
  }

  randomSeed(esp_random());

  drawMenu();
}

void loop() {
  EventType upEvt     = pollButton(bUp);
  EventType downEvt   = pollButton(bDown);
  EventType selectEvt = pollButton(bSelect);

  if (screen == SCREEN_MENU) {
    if (upEvt == EVT_PRESS || upEvt == EVT_REPEAT) {
      menuIndex = (menuIndex - 1 + MENU_COUNT) % MENU_COUNT;
      drawMenu();
    }
    if (downEvt == EVT_PRESS || downEvt == EVT_REPEAT) {
      menuIndex = (menuIndex + 1) % MENU_COUNT;
      drawMenu();
    }
    if (selectEvt == EVT_PRESS) {
      enterPage(menuIndex);
    }
    delay(5);
    return;
  }

  // SCREEN_PAGE
  switch (activePage) {
    case 0: pageSnow(upEvt, downEvt, selectEvt); break;
    case 1: pageSystemInfo(upEvt, downEvt, selectEvt); break;
    case 2: pageI2CScan(upEvt, downEvt, selectEvt); break;
    case 3: pageSettings(upEvt, downEvt, selectEvt); break;
  }

  delay(5);
}
