#pragma once
#include <Arduino.h>

namespace Input {
    void begin();
    void update(uint32_t now);
    bool buttonPressed();
}
