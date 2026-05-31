#include "ExperimentRunner.h"
#include "SOCEstimator.h"
#include "EventLogger.h"
#include "DataExporter.h"
#include "../include/constants.h"

ExperimentRunner experimentRunner;

void ExperimentRunner::begin() {
    _status = {};
    _status.type = EXP_NONE;
    _status.running = false;
}

bool ExperimentRunner::start(ExperimentType type, float param) {
    if (_status.running) {
        Serial.println("[Experiment] Already running, stop first");
        return false;
    }

    _status = {};
    _status.type = type;
    _status.running = true;
    _status.startTime_ms = millis();
    _status.elapsed_s = 0;
    _status.energy_in_Wh = 0;
    _status.energy_out_Wh = 0;
    _status.startSOC = soc.getSOC();
    _status.currentSOC = _status.startSOC;
    _status.samples = 0;

    // Experiment-specific setup
    switch (type) {
        case EXP_E5_SOC_VERIFY:
            _status.e5Phase = E5_DISCHARGE_TO_EMPTY;
            break;
        case EXP_E6_AUTONOMY:
            _status.targetSOCmin = param;  // 20/30/45/60/70
            break;
        default:
            break;
    }

    // Start fresh CSV file for this experiment
    DataExporter::startNew(getCurrentName());

    eventLog.add(EVT_EXPERIMENT_START, getCurrentName());
    Serial.printf("[Experiment] STARTED: %s (param=%.1f)\n", getCurrentName(), param);
    return true;
}

void ExperimentRunner::stop() {
    if (!_status.running) return;

    Serial.printf("[Experiment] STOPPED: %s\n", getCurrentName());
    Serial.printf("  Elapsed: %lu s\n", (unsigned long)_status.elapsed_s);
    Serial.printf("  E_in:    %.3f Wh\n", _status.energy_in_Wh);
    Serial.printf("  E_out:   %.3f Wh\n", _status.energy_out_Wh);
    Serial.printf("  SOC:     %.1f%% -> %.1f%% (Δ%+.1f)\n",
        _status.startSOC, _status.currentSOC, _status.deltaSOC);
    Serial.printf("  Samples: %lu\n", (unsigned long)_status.samples);

    eventLog.add(EVT_EXPERIMENT_STOP, getCurrentName());
    DataExporter::close();

    _status.running = false;
    _status.type = EXP_NONE;
}

void ExperimentRunner::tick(const SensorData& d, float dt_s) {
    if (!_status.running) return;

    _status.elapsed_s = (millis() - _status.startTime_ms) / 1000;
    _status.currentSOC = soc.getSOC();
    _status.deltaSOC = _status.currentSOC - _status.startSOC;
    _status.samples++;

    // Energy accounting (P = U·I, ΔE = P·Δt)
    float P_charge = d.V_bat * d.I_charge;   // W
    float P_load   = d.V_load * d.I_load;             // W

    _status.energy_in_Wh  += P_charge * dt_s / 3600.0f;
    _status.energy_out_Wh += P_load   * dt_s / 3600.0f;

    // Dispatch to experiment-specific logic
    switch (_status.type) {
        case EXP_E1_USB_CHARGE: handleE1(d, dt_s); break;
        case EXP_E2_PV_SUNNY:
        case EXP_E3_PV_CLOUDY:  handleE2or3(d, dt_s); break;
        case EXP_E4_DISCHARGE:  handleE4(d, dt_s); break;
        case EXP_E5_SOC_VERIFY: handleE5(d, dt_s); break;
        case EXP_E6_AUTONOMY:   handleE6(d, dt_s); break;
        default: break;
    }

    // Log every sample to CSV
    logSample(d);
}

void ExperimentRunner::handleE1(const SensorData& d, float dt_s) {
    // E1: Charge from stable 5V USB
    // Stop when SOC >= 100% AND I_charge drops to cutoff (CV phase ends)
    if (_status.currentSOC >= 99.5f && d.I_charge < 0.05f) {
        Serial.println("[E1] Full charge reached");
        soc.setSOC(100.0f);
        stop();
    }
}

void ExperimentRunner::handleE2or3(const SensorData& d, float dt_s) {
    // E2/E3: Charge from PV (sunny/cloudy)
    // Stop on full charge OR on no current >5 min (sun set, dark)
    static uint32_t no_current_start = 0;

    if (_status.currentSOC >= 99.5f && d.I_charge < 0.05f) {
        Serial.println("[E2/E3] Full charge reached");
        soc.setSOC(100.0f);
        stop();
        return;
    }

    if (d.I_charge < 0.02f) {
        if (no_current_start == 0) no_current_start = millis();
        else if ((millis() - no_current_start) > 5 * 60 * 1000UL) {
            Serial.println("[E2/E3] No PV current for 5 min, stopping");
            stop();
            no_current_start = 0;
        }
    } else {
        no_current_start = 0;
    }
}

void ExperimentRunner::handleE4(const SensorData& d, float dt_s) {
    // E4: Discharge to load until V_bat <= U_MIN
    if (d.V_load <= U_MIN || _status.currentSOC <= 1.0f) {
        Serial.println("[E4] Discharge cutoff reached");
        soc.setSOC(0.0f);
        stop();
    }
}

void ExperimentRunner::handleE5(const SensorData& d, float dt_s) {
    // E5: Full cycle (discharge -> charge -> discharge) for SOC verification
    switch (_status.e5Phase) {
        case E5_DISCHARGE_TO_EMPTY:
            if (d.V_load <= U_MIN || _status.currentSOC <= 1.0f) {
                Serial.println("[E5] Phase 1 done: discharged to empty");
                soc.setSOC(0.0f);
                _status.e5_full_discharge_ms = millis();
                _status.e5Phase = E5_CHARGE_TO_FULL;
                eventLog.add(EVT_E5_PHASE, "discharge1 -> charge");
            }
            break;

        case E5_CHARGE_TO_FULL:
            if (_status.currentSOC >= 99.5f && d.I_charge < 0.05f) {
                Serial.println("[E5] Phase 2 done: charged to full");
                soc.setSOC(100.0f);
                _status.e5_full_charge_ms = millis();
                _status.e5Phase = E5_DISCHARGE_AGAIN;
                eventLog.add(EVT_E5_PHASE, "charge -> discharge2");
            }
            break;

        case E5_DISCHARGE_AGAIN:
            if (d.V_load <= U_MIN || _status.currentSOC <= 1.0f) {
                Serial.println("[E5] Phase 3 done: complete");
                _status.e5Phase = E5_DONE;
                eventLog.add(EVT_E5_PHASE, "verification complete");
                stop();
            }
            break;

        default:
            break;
    }
}

void ExperimentRunner::handleE6(const SensorData& d, float dt_s) {
    // E6: Discharge until SOC reaches SOC_CRITICAL (10%)
    // Useful: time elapsed from targetSOCmin -> SOC_CRITICAL
    if (_status.currentSOC <= SOC_CRITICAL) {
        Serial.printf("[E6] Reached critical SOC from initial=%.1f%%\n",
                      _status.startSOC);
        Serial.printf("[E6] Autonomy time: %lu seconds = %.2f hours\n",
                      (unsigned long)_status.elapsed_s,
                      _status.elapsed_s / 3600.0f);
        stop();
    }
}

const char* ExperimentRunner::getCurrentName() const {
    switch (_status.type) {
        case EXP_E1_USB_CHARGE: return "E1_USB_charge";
        case EXP_E2_PV_SUNNY:   return "E2_PV_sunny";
        case EXP_E3_PV_CLOUDY:  return "E3_PV_cloudy";
        case EXP_E4_DISCHARGE:  return "E4_discharge";
        case EXP_E5_SOC_VERIFY: return "E5_SOC_verify";
        case EXP_E6_AUTONOMY:   return "E6_autonomy";
        default:                return "none";
    }
}

void ExperimentRunner::logSample(const SensorData& d) {
    DataExporter::writeRow(_status.elapsed_s, d, _status.currentSOC,
                            _status.energy_in_Wh, _status.energy_out_Wh);
}
