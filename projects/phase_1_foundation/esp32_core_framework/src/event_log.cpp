#include "event_log.h"

namespace EventLog {

void begin() {}

void push(const char* msg) {
    Serial.printf("EVENT: %s\n", msg);
}

}
