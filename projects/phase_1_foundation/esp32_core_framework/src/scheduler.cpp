#include "scheduler.h"
#include "sensors.h"
#include <Arduino.h>

namespace {
    uint32_t tHeartbeat = 0;
    const uint32_t HEARTBEAT_MS = 1000;
}

namespace Scheduler {

void update(uint32_t now) {
    if (now - tHeartbeat >= HEARTBEAT_MS) {
        tHeartbeat = now;

        Serial.printf("HB | uptime: %lu ms\n", (unsigned long)now);

        if (Sensors::hasDistance()) {
            Serial.printf("DIST: %.1f cm\n", Sensors::distanceCm());
        } else {
            Serial.println("DIST: ---");
        }
    }
}

}
