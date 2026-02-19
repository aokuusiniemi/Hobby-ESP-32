#pragma once
#include <Arduino.h>

namespace Sensors {
    void begin();
    void update(uint32_t now);

    bool hasDistance();
    float distanceCm();
}
