// =====================================================
// DIAGNOSTIC SKETCH for TCA9548A + INA3221 + INA219
// Paste this as src/main.cpp temporarily (and remove all
// other .cpp files from src/ to avoid 'multiple definition' errors,
// or set build_src_filter = +<main.cpp> in platformio.ini)
// =====================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SDL_Arduino_INA3221.h>

#define SDA_PIN       21
#define SCL_PIN       22
#define TCA_ADDR      0x70
#define INA_ADDR      0x40

Adafruit_INA219 ina219(INA_ADDR);
SDL_Arduino_INA3221 ina3221(INA_ADDR);

void tcaSelect(uint8_t ch) {
    Wire.beginTransmission(TCA_ADDR);
    Wire.write(1 << ch);
    Wire.endTransmission();
    delay(2);
}

void scanI2C(const char* label) {
    Serial.printf("\n--- Scan %s ---\n", label);
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  0x%02X\n", addr);
            found++;
        }
    }
    Serial.printf("  Found: %d devices\n", found);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TCA9548A + INA Diagnostic ===");

    Wire.begin(SDA_PIN, SCL_PIN, 100000);
    delay(100);

    // Step 1: bare bus (without any TCA channel selected)
    scanI2C("BARE bus (no TCA channel)");
    Serial.println("Expected: 0x70 (TCA9548A)");

    // Step 2: channel 0 (INA3221)
    tcaSelect(0);
    scanI2C("TCA channel 0 (INA3221 expected)");
    Serial.println("Expected: 0x40 (INA3221) + 0x70");

    // Step 3: channel 1 (INA219)
    tcaSelect(1);
    scanI2C("TCA channel 1 (INA219 expected)");
    Serial.println("Expected: 0x40 (INA219) + 0x70");

    // Step 4: try to actually read INA219
    Serial.println("\n--- INA219 communication test ---");
    tcaSelect(1);
    if (ina219.begin()) {
        ina219.setCalibration_16V_400mA();
        Serial.println("INA219 begin() OK");
    } else {
        Serial.println("INA219 begin() FAILED");
    }

    // Step 5: try to init INA3221
    Serial.println("\n--- INA3221 communication test ---");
    tcaSelect(0);
    ina3221.begin();
    Serial.println("INA3221 begin() called (no return value)");

    Serial.println("\n=== Live readings ===");
}

void loop() {
    Serial.println("------------------------------");

    // INA3221 (TCA channel 0)
    tcaSelect(0);
    delay(2);
    for (int ch = 1; ch <= 3; ch++) {
        float V = ina3221.getBusVoltage_V(ch);
        float I = ina3221.getCurrent_mA(ch);
        Serial.printf("INA3221 ch%d : V=%6.3f V    I=%+8.2f mA  %s\n",
                      ch, V, I, (V > 0.5f) ? "<-- ACTIVE" : "");
    }

    // INA219 (TCA channel 1)
    tcaSelect(1);
    delay(2);
    float V = ina219.getBusVoltage_V();
    float I = ina219.getCurrent_mA();
    Serial.printf("INA219      : V=%6.3f V    I=%+8.2f mA  (battery line)\n", V, I);

    delay(1000);
}
