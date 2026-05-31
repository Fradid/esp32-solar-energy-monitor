// =====================================================
// DIAGNOSTIC SKETCH — paste this as main.cpp temporarily
// to debug sensor wiring before running real experiments.
// =====================================================
// What it does:
//   1. Scans I²C bus and lists every responding address
//   2. Reads all 3 channels of INA3221 and prints V, I
//   3. Reads INA219 and prints V, I
//   4. Highlights which channel actually carries voltage
//
// Use this to verify: which INA3221 channel is your solar input

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SDL_Arduino_INA3221.h>

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_INA219 ina219(0x41);
SDL_Arduino_INA3221 ina3221(0x40);

void scanI2C() {
    Serial.println("\n=== I²C bus scan ===");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Device at 0x%02X\n", addr);
            found++;
        }
    }
    Serial.printf("Total found: %d\n", found);
    if (found == 0) {
        Serial.println("!! No I²C devices. Check SDA=21, SCL=22, GND, 3V3 connections.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n========================================");
    Serial.println("Battery rig diagnostic sketch");
    Serial.println("========================================");

    Wire.begin(SDA_PIN, SCL_PIN, 100000);
    delay(100);

    scanI2C();

    if (!ina219.begin()) {
        Serial.println("[FAIL] INA219 not responding at 0x41");
    } else {
        ina219.setCalibration_16V_400mA();
        Serial.println("[OK] INA219 ready (16V/400mA cal)");
    }

    ina3221.begin();
    Wire.beginTransmission(0x40);
    if (Wire.endTransmission() != 0) {
        Serial.println("[FAIL] INA3221 not responding at 0x40");
    } else {
        Serial.println("[OK] INA3221 ready");
    }

    Serial.println("\n=== Live readings (every 1 s) ===");
    Serial.println("Watch which INA3221 channel shows non-zero voltage");
    Serial.println("That's where your solar panel is actually connected.\n");
}

void loop() {
    Serial.println("--------------------------------------");

    // INA3221 — all 3 channels
    for (int ch = 1; ch <= 3; ch++) {
        float V = ina3221.getBusVoltage_V(ch);
        float I_mA = ina3221.getCurrent_mA(ch);
        Serial.printf("INA3221 ch%d : V = %6.3f V    I = %+8.2f mA   %s\n",
                      ch, V, I_mA,
                      (V > 0.5f) ? "<-- ACTIVE" : "");
    }

    // INA219
    float V219 = ina219.getBusVoltage_V();
    float I219 = ina219.getCurrent_mA();
    Serial.printf("INA219       : V = %6.3f V    I = %+8.2f mA   (battery line)\n",
                  V219, I219);

    delay(1000);
}
