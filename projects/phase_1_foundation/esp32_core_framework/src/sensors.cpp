#include "sensors.h"

namespace {

const uint8_t TRIG_PIN = 13;
const uint8_t ECHO_PIN = 18;

uint32_t tSample = 0;
const uint32_t SAMPLE_MS = 150;

float buf[5] = {0};
uint8_t idx = 0;
uint8_t count = 0;

float lastCm = -1.0f;
bool valid = false;

// returns distance in cm, or -1 on timeout/error
float readHCSR04cm() {
    // Ensure clean low pulse
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    // 10us trigger pulse
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // pulseIn returns microseconds of HIGH
    // timeout ~25ms => ~4m max range
    uint32_t us = pulseIn(ECHO_PIN, HIGH, 25000);

    if (us == 0) return -1.0f;

    // Speed of sound ~343 m/s => 29.1 us/cm round trip => /58 approx
    return us / 58.0f;
}

}

namespace Sensors {

void begin() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);

    lastCm = -1.0f;
    valid = false;
    // reset rolling buffer
    for (uint8_t i = 0; i < 5; i++) buf[i] = 0.0f;
    idx = 0;
    count = 0;
}

void update(uint32_t now) {
    if (now - tSample < SAMPLE_MS) return;
    tSample = now;

    float cm = readHCSR04cm();
    if (cm <= 0 || cm >= 400) { valid = false; return; }

    buf[idx] = cm;
    idx = (idx + 1) % 5;
    if (count < 5) count++;

    float sum = 0;
    for (uint8_t i = 0; i < count; i++) sum += buf[i];
    lastCm = sum / count;
    valid = true;
}

bool hasDistance() { return valid; }
float distanceCm() { return lastCm; }

}
