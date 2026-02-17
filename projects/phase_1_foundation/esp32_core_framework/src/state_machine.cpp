#include "state_machine.h"
#include "event_log.h"

namespace {

enum class State {
    IDLE,
    ACTIVE
};

State current = State::IDLE;
uint32_t tToggle = 0;
const uint32_t TOGGLE_MS = 5000;

void setState(State next) {
    if (next == current) return;

    current = next;

    if (current == State::IDLE)
        EventLog::push("STATE: IDLE");
    else
        EventLog::push("STATE: ACTIVE");
}

}

namespace StateMachine {

void begin() {
    current = State::IDLE;
    EventLog::push("STATE: INIT -> IDLE");
}

void update(uint32_t now) {

    if (now - tToggle >= TOGGLE_MS) {
        tToggle = now;

        if (current == State::IDLE)
            setState(State::ACTIVE);
        else
            setState(State::IDLE);
    }

}

}
