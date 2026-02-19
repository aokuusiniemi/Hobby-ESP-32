#include "input.h"

namespace {
const uint8_t BUTTON_PIN = 4;          // change if needed
const uint32_t DEBOUNCE_MS = 30;

bool stableState = HIGH;               // INPUT_PULLUP => HIGH = not pressed
bool lastRead = HIGH;

uint32_t lastChange = 0;
bool pressedEvent = false;
}

namespace Input {

void begin() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void update(uint32_t now) {
    bool reading = digitalRead(BUTTON_PIN);

    if (reading != lastRead) {
        lastChange = now;
        lastRead = reading;
    }

    // If the reading has been stable for DEBOUNCE_MS, accept it as stable
    if ((now - lastChange) >= DEBOUNCE_MS && reading != stableState) {
        stableState = reading;

        // Fire event on press (HIGH -> LOW)
        if (stableState == LOW) {
            pressedEvent = true;
        }
    }
}

bool buttonPressed() {
    if (pressedEvent) {
        pressedEvent = false;
        return true;
    }
    return false;
}

}
