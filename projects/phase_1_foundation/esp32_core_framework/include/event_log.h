#pragma once
#include <Arduino.h>

namespace EventLog {
    void begin();
    void push(const char* msg);
}
