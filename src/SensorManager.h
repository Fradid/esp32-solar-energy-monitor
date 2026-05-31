#pragma once
#include <Arduino.h>

// =====================================================
// SensorManager.h - INA3221 (charge) + INA219 (battery+discharge)
// =====================================================
// Hardware reality:
//   - INA3221 channel 1: measures whichever source is active
//     (PV solar panel OR USB micro-USB charger, via ESP8266 bridge)
//   - INA219: sits directly on the battery line, measures
//     V_bat AND discharge current to DC-DC converter

struct SensorData {
    // INA3221 channel 1: active charge source
    float V_charge;    // V, voltage from PV or USB (whichever is connected)
    float I_charge;    // A, signed (>0 = current into battery)

    // INA219: battery + load
    float V_load;      // V, battery voltage (since INA219 is on battery line)
    float I_load;      // A, signed (>0 = discharge)

    // Derived
    float V_bat;       // V, battery voltage (from INA219)

    // Status
    bool ina3221_ok;
    bool ina219_ok;
    uint32_t timestamp_ms;
};

class SensorManager {
public:
    bool begin();
    SensorData readAll();
    bool isHealthy() const { return _healthy; }

private:
    bool _healthy = false;
    SensorData _lastValid{};
};

extern SensorManager sensors;
