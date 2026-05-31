#pragma once
#include <Arduino.h>

// =====================================================
// SOCEstimator.h - Combined coulomb counting + OCV correction
// =====================================================

class SOCEstimator {
public:
    void begin(float initialSOC = 50.0f);

    /**
     * Update SOC estimate from current measurements.
     * @param I_charge  Net charge current in A (>0 charging)
     * @param I_load    Discharge current in A (>0 discharging)
     * @param V_bat     Battery voltage in V (for OCV correction)
     * @param dt_s      Time step in seconds
     */
    void update(float I_charge, float I_load, float V_bat, float dt_s);

    /**
     * Manually set SOC (e.g., after known full charge or full discharge).
     */
    void setSOC(float soc);

    float getSOC() const { return _soc; }
    float getChargedAh() const { return _accumulated_As / 3600.0f; }
    void resetAccumulator() { _accumulated_As = 0; }

private:
    float _soc = 50.0f;
    float _accumulated_As = 0.0f;     // Integrated charge in coulombs
    uint32_t _idleStart_ms = 0;
    bool _isIdle = false;

    float lookupSOCfromV(float V);
};

extern SOCEstimator soc;
