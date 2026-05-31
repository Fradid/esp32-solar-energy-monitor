#include "SOCEstimator.h"
#include "../include/constants.h"

SOCEstimator soc;

// 12-point lookup table: U(V) -> SOC(%) for Li-Ion 18650 at rest
struct OcvPoint { float V; float SOC; };
constexpr OcvPoint OCV_TABLE[12] = {
    {3.00f,   0.0f},
    {3.20f,   5.0f},
    {3.40f,  10.0f},
    {3.55f,  20.0f},
    {3.65f,  35.0f},
    {3.70f,  50.0f},
    {3.75f,  65.0f},
    {3.80f,  75.0f},
    {3.90f,  85.0f},
    {4.00f,  92.0f},
    {4.10f,  97.0f},
    {4.20f, 100.0f},
};

void SOCEstimator::begin(float initialSOC) {
    _soc = constrain(initialSOC, 0.0f, 100.0f);
    _accumulated_As = 0.0f;
    _idleStart_ms = 0;
    _isIdle = false;
}

void SOCEstimator::setSOC(float s) {
    _soc = constrain(s, 0.0f, 100.0f);
}

float SOCEstimator::lookupSOCfromV(float V) {
    if (V <= OCV_TABLE[0].V) return OCV_TABLE[0].SOC;
    if (V >= OCV_TABLE[11].V) return OCV_TABLE[11].SOC;
    for (int i = 1; i < 12; i++) {
        if (V <= OCV_TABLE[i].V) {
            float v1 = OCV_TABLE[i-1].V;
            float v2 = OCV_TABLE[i].V;
            float s1 = OCV_TABLE[i-1].SOC;
            float s2 = OCV_TABLE[i].SOC;
            return s1 + (s2 - s1) * (V - v1) / (v2 - v1);
        }
    }
    return _soc;
}

void SOCEstimator::update(float I_charge, float I_load, float V_bat, float dt_s) {
    // Net current: positive = charging, negative = discharging
    float I_net = I_charge - I_load;

    // Choose efficiency by direction
    float eta = (I_net > 0) ? ETA_CHARGE : ETA_DISCHARGE;

    // Coulomb counting
    float dQ_As = eta * I_net * dt_s;            // delta charge in coulombs
    _accumulated_As += dQ_As;

    float dSOC = (dQ_As / Q_NOM_AS) * 100.0f;     // % change
    _soc = constrain(_soc + dSOC, 0.0f, 100.0f);

    // OCV correction logic
    bool currentlyIdle = (fabs(I_net) < I_IDLE_THR_A);
    if (currentlyIdle) {
        if (!_isIdle) {
            _idleStart_ms = millis();
            _isIdle = true;
        } else if ((millis() - _idleStart_ms) > IDLE_TOUT_MS) {
            // Idle long enough — apply OCV-based correction
            // Only at extremes where U(SOC) has steep slope
            if (_soc > 95.0f || _soc < 15.0f) {
                float soc_ocv = lookupSOCfromV(V_bat);
                _soc = (1.0f - OCV_WEIGHT) * _soc + OCV_WEIGHT * soc_ocv;
            }
        }
    } else {
        _isIdle = false;
    }
}
