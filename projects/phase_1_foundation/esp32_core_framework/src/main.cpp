#include <Arduino.h>
#include "scheduler.h"
#include "state_machine.h"
#include "event_log.h"

void setup() {
    Serial.begin(115200);
    delay(200);

    EventLog::begin();
    StateMachine::begin();

    EventLog::push("BOOT");
    Serial.println("Core framework initialized");
}

void loop() {
    uint32_t now = millis();

    Scheduler::update(now);
    StateMachine::update(now);
}
