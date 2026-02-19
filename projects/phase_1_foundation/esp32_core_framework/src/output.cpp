#include "output.h"

namespace {

const uint8_t LED_PIN = 2;

bool activeMode = false;
uint32_t blinkTimer = 0;
bool ledState = false;

}

namespace Output {

void begin() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void setIdle() {
    activeMode = false;
    digitalWrite(LED_PIN, LOW);
}

void setActive() {
    activeMode = true;
}

void update(uint32_t now) {

    if (!activeMode) return;

    if (now - blinkTimer >= 250) {
        blinkTimer = now;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
    }
}

}
