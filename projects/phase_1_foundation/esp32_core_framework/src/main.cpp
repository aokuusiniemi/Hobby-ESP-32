#include <Arduino.h>
#include "scheduler.h"
#include "state_machine.h"
#include "event_log.h"
#include "input.h"
#include "output.h"
#include "sensors.h"


void setup() {
    Serial.begin(115200);
    delay(200);

    EventLog::begin();
    StateMachine::begin();

    EventLog::push("BOOT");
    Serial.println("Core framework initialized");

    Input::begin();
    Sensors::begin();
    Output::begin();

}

void loop() {
    uint32_t now = millis();

    Scheduler::update(now);
    Input::update(now);
    Sensors::update(now);
    StateMachine::update(now);
    Output::update(now);
}


