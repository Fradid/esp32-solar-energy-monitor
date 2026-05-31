#include "SensorManager.h"
#include "TCA9548.h"
#include "../include/pin_config.h"
#include "../include/constants.h"
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SDL_Arduino_INA3221.h>

Adafruit_INA219 ina219(ADDR_INA219);
SDL_Arduino_INA3221 ina3221(ADDR_INA3221);
SensorManager sensors;

static bool _ina3221_ok = false;
static bool _ina219_ok  = false;

bool SensorManager::begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQUENCY);
    delay(50);

    // Step 1: TCA9548 multiplexer
    if (!tca.begin(ADDR_TCA9548)) {
        Serial.println("[Sensors] TCA9548 not found — sensors will not work");
        _healthy = false;
        return false;
    }

    // Step 2: Initialize INA3221 (on TCA channel 0)
    tca.selectChannel(TCA_CH_INA3221);
    delay(10);
    ina3221.begin();
    Wire.beginTransmission(ADDR_INA3221);
    _ina3221_ok = (Wire.endTransmission() == 0);
    Serial.printf("[Sensors] INA3221 (TCA ch%d): %s\n",
                  TCA_CH_INA3221, _ina3221_ok ? "OK" : "NOT FOUND");

    // Step 3: Initialize INA219 (on TCA channel 1)
    tca.selectChannel(TCA_CH_INA219);
    delay(10);
    _ina219_ok = ina219.begin();
    if (_ina219_ok) {
        ina219.setCalibration_16V_400mA();
        Serial.printf("[Sensors] INA219 (TCA ch%d): OK (16V/400mA cal)\n", TCA_CH_INA219);
    } else {
        Serial.printf("[Sensors] INA219 (TCA ch%d): NOT FOUND\n", TCA_CH_INA219);
    }

    _healthy = _ina3221_ok && _ina219_ok;
    return _healthy;
}

SensorData SensorManager::readAll() {
    SensorData d{};

    // === Read INA3221 (TCA channel 0) ===
    if (_ina3221_ok) {
        tca.selectChannel(TCA_CH_INA3221);
        d.V_charge = ina3221.getBusVoltage_V(INA3221_CH_CHARGE);
        d.I_charge = ina3221.getCurrent_mA(INA3221_CH_CHARGE) / 1000.0f;
        d.ina3221_ok = true;
    }

    // === Read INA219 (TCA channel 1) ===
    if (_ina219_ok) {
        tca.selectChannel(TCA_CH_INA219);
        d.V_load = ina219.getBusVoltage_V();
        d.I_load = ina219.getCurrent_mA() / 1000.0f;
        d.ina219_ok = true;
    }

    // V_bat: prefer INA219 (sits directly on battery line)
    if (_ina219_ok && d.V_load > 2.5f) {
        d.V_bat = d.V_load;
    } else if (_ina3221_ok && d.V_charge > 2.5f) {
        d.V_bat = d.V_charge;
    } else {
        d.V_bat = 0.0f;
    }

    d.timestamp_ms = millis();

    // Sanity check for single Li-Ion cell (2.5...4.5V working range)
    if (d.V_bat < 2.5f || d.V_bat > 4.5f) {
        Serial.printf("[Sensors] Suspect V_bat=%.2f V_load=%.2f V_charge=%.2f I_charge=%.3f I_load=%.3f\n",
                      d.V_bat, d.V_load, d.V_charge, d.I_charge, d.I_load);
        if (_lastValid.V_bat > 2.5f) {
            float saved_vc = d.V_charge;
            float saved_ic = d.I_charge;
            d = _lastValid;
            d.V_charge = saved_vc;
            d.I_charge = saved_ic;
            d.timestamp_ms = millis();
        }
    } else {
        _lastValid = d;
    }

    return d;
}
