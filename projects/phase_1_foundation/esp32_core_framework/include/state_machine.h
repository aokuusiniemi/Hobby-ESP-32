#pragma once
#include <Arduino.h>

namespace StateMachine {

enum class State : uint8_t {
    IDLE,
    ACTIVE,
    AVOIDING
};

void begin();
void update(uint32_t now);
State getState();
// Returns true once when a state transition occurs; subsequent calls
// return false until the next transition.
bool stateChanged();

}
