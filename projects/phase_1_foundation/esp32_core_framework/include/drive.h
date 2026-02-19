#pragma once
#include <Arduino.h>

namespace Drive {
  void begin();
  void forward(uint8_t speedPct);
  void reverse(uint8_t speedPct);
  void stop();
  void turnLeft(uint8_t speedPct);
  void turnRight(uint8_t speedPct);
}
