#pragma once
#include <Arduino.h>

namespace Output {
    void begin();
    void setIdle();
    void setActive();
    void update(uint32_t now);
}
