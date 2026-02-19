#include "drive.h"

namespace {

enum class Mode : uint8_t {
    STOP,
    FWD,
    REV,
    LEFT,
    RIGHT
};

Mode currentMode = Mode::STOP;
uint8_t currentSpeed = 0;

void logIfChanged(Mode newMode, uint8_t speedPct) {
    if (newMode == currentMode && speedPct == currentSpeed) return;

    currentMode = newMode;
    currentSpeed = speedPct;

    switch (currentMode) {
        case Mode::STOP:
            Serial.println("DRIVE: stop");
            break;
        case Mode::FWD:
            Serial.printf("DRIVE: forward %u%%\n", currentSpeed);
            break;
        case Mode::REV:
            Serial.printf("DRIVE: reverse %u%%\n", currentSpeed);
            break;
        case Mode::LEFT:
            Serial.printf("DRIVE: left %u%%\n", currentSpeed);
            break;
        case Mode::RIGHT:
            Serial.printf("DRIVE: right %u%%\n", currentSpeed);
            break;
    }
}

}

namespace Drive {

void begin() {
    currentMode = Mode::STOP;
    currentSpeed = 0;
}

void forward(uint8_t speedPct)   { logIfChanged(Mode::FWD, speedPct); }
void reverse(uint8_t speedPct)   { logIfChanged(Mode::REV, speedPct); }
void stop()                      { logIfChanged(Mode::STOP, 0); }
void turnLeft(uint8_t speedPct)  { logIfChanged(Mode::LEFT, speedPct); }
void turnRight(uint8_t speedPct) { logIfChanged(Mode::RIGHT, speedPct); }

}
