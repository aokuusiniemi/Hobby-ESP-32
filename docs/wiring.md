# Wiring

ESP32 DevKit V1 → OLED 128x64 (I2C)

- 3V3  → VCC
- GND  → GND
- GPIO21 (SDA) → SDA
- GPIO22 (SCL) → SCL

Code uses: `Wire.begin(21, 22);`
