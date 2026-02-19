#include "state_machine.h"
#include "event_log.h"
#include "output.h"
#include "sensors.h"
#include "drive.h"

// Implementation notes:
// - This file contains the concrete state machine implementation used by
//   the application. Internal variables and helpers are kept in an
//   anonymous namespace so callers must use the `StateMachine` API.
// - The state machine prefers automatic state updates from the distance
//   sensor when available.
namespace {

// The current, authoritative state for the system. Defined using the
// `State` enum from the header to keep the public API consistent.
StateMachine::State current = StateMachine::State::IDLE;

// Edge flag: set when a transition happens. `stateChanged()` consumes
// and clears this flag so callers get a single true per transition.
bool stateChangedFlag = false;

// Hysteresis thresholds for distance sensor (cm).
// ENTER: transition to ACTIVE when distance drops below this
// EXIT: transition to IDLE when distance rises above this
const float THRESH_ENTER_CM = 20.0f;
const float THRESH_EXIT_CM = 25.0f;

// Avoidance behavior step sequence.
enum class AvoidStep : uint8_t { REV, TURN };

// Avoidance state tracking.
AvoidStep avoidStep = AvoidStep::REV;
uint32_t avoidStart = 0;

// Avoidance timing (ms).
const uint32_t REV_MS  = 400;
const uint32_t TURN_MS = 500;

// Cooldown / re-arm after avoidance
uint32_t avoidCooldownUntil = 0;
const uint32_t AVOID_COOLDOWN_MS = 1200;
bool avoidArmed = true;

// Convert a state value to a short, human-readable name for logging.
const char* stateName(StateMachine::State s) {
    switch (s) {
        case StateMachine::State::IDLE:    return "IDLE";
        case StateMachine::State::ACTIVE:  return "ACTIVE";
        case StateMachine::State::AVOIDING: return "AVOIDING";
        default:                            return "UNKNOWN";
    }
}

// Update the current state and perform side-effects (logging + outputs).
// If `next` equals the current state no action is performed.
void setState(StateMachine::State next) {
    if (next == current) return;

    current = next;
    stateChangedFlag = true;

    // Record the transition using a concise state name. Keeping the log
    // entry limited to the state name makes future refactors (adding
    // states) simpler while preserving readable logs.
    EventLog::push(stateName(current));

    // Side-effect: update physical outputs to reflect the new state.
    if (current == StateMachine::State::IDLE) {
        Output::setIdle();
    } else if (current == StateMachine::State::ACTIVE) {
        Output::setActive();
    }
    // AVOIDING state doesn't change Output; drive commands handle behavior.
}

// Initiate the avoidance sequence: first REV, then TURN.
void enterAvoid(uint32_t now) {
    avoidStart = now;
    avoidStep = AvoidStep::REV;
    Drive::reverse(40);
    // disarm until we see clear distance after the maneuver
    avoidArmed = false;
    setState(StateMachine::State::AVOIDING);
}

} // namespace

namespace StateMachine {

void begin() {
    current = StateMachine::State::IDLE;
    stateChangedFlag = false;

    EventLog::push("STATE: INIT -> IDLE");
    Output::setIdle();
    Drive::forward(40);
}

void update(uint32_t now) {

    // If we don't have a distance reading, don't change state.
    // (Keep current drive command.)
    if (!Sensors::hasDistance() && current == StateMachine::State::IDLE) {
    Drive::forward(40);
    return;
    }   

        if (current == StateMachine::State::IDLE) {
            // If no distance reading, stay IDLE.
            if (!Sensors::hasDistance()) return;

            // Respect cooldown period after an avoid maneuver
            if (now < avoidCooldownUntil) {
                Drive::forward(40);
                return;
            }

            const float cm = Sensors::distanceCm();

            // If avoidance is not armed, wait until distance clears above exit threshold
            if (!avoidArmed) {
                if (cm > THRESH_EXIT_CM) avoidArmed = true;
                Drive::forward(40);
                return;
            }

            // Enter avoid when close
            if (cm <= THRESH_ENTER_CM) {
                enterAvoid(now);
            } else {
                // Keep cruising
                Drive::forward(40);
            }
            return;
        }

    if (current == StateMachine::State::AVOIDING) {
    const uint32_t elapsed = now - avoidStart;

    if (avoidStep == AvoidStep::REV) {
        if (elapsed >= REV_MS) {
            avoidStep = AvoidStep::TURN;
            avoidStart = now; // reset timing for TURN step
            Drive::turnRight(40);
        }
        return;
    }

    // TURN step
    if (elapsed >= TURN_MS) {
        avoidCooldownUntil = now + AVOID_COOLDOWN_MS;
        Drive::forward(40);
        setState(StateMachine::State::IDLE);
    }
    return;
 } 
}

StateMachine::State getState() {
    return current;
}

bool stateChanged() {
    bool v = stateChangedFlag;
    stateChangedFlag = false;
    return v;
}

} // namespace StateMachine
