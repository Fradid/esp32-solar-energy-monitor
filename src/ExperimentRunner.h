#pragma once
#include <Arduino.h>
#include "SensorManager.h"

// =====================================================
// ExperimentRunner.h - Manages 6 experiment scenarios
// =====================================================

enum ExperimentType {
    EXP_NONE          = 0,
    EXP_E1_USB_CHARGE = 1,    // Charge from stable 5V USB
    EXP_E2_PV_SUNNY   = 2,    // Charge from PV (direct sun)
    EXP_E3_PV_CLOUDY  = 3,    // Charge from PV (cloudy)
    EXP_E4_DISCHARGE  = 4,    // Discharge to load
    EXP_E5_SOC_VERIFY = 5,    // SOC accuracy verification (full cycle)
    EXP_E6_AUTONOMY   = 6,    // Autonomy at different initial SOC
};

enum E5Phase {
    E5_IDLE    = 0,
    E5_DISCHARGE_TO_EMPTY = 1,
    E5_CHARGE_TO_FULL     = 2,
    E5_DISCHARGE_AGAIN    = 3,
    E5_DONE    = 4,
};

struct ExperimentStatus {
    ExperimentType type;
    bool running;
    uint32_t startTime_ms;
    uint32_t elapsed_s;

    // Energy accounting
    float energy_in_Wh;       // Cumulative energy in
    float energy_out_Wh;      // Cumulative energy out
    float currentSOC;
    float startSOC;
    float deltaSOC;

    // For E6: target SOC_min (set before start)
    float targetSOCmin;

    // For E5: current phase
    E5Phase e5Phase;

    // For E5: full charge / full discharge timestamps
    uint32_t e5_full_charge_ms;
    uint32_t e5_full_discharge_ms;

    // Sample count (for CSV export)
    uint32_t samples;
};

class ExperimentRunner {
public:
    void begin();

    bool start(ExperimentType type, float param = 0);
    void stop();

    /**
     * Called once per second from monitor task.
     * Updates energy integrals, checks completion criteria.
     */
    void tick(const SensorData& data, float dt_s);

    const ExperimentStatus& getStatus() const { return _status; }
    bool isRunning() const { return _status.running; }

    /**
     * Get experiment name for UI / logs / CSV.
     */
    const char* getCurrentName() const;

private:
    ExperimentStatus _status{};

    void handleE1(const SensorData& d, float dt_s);
    void handleE2or3(const SensorData& d, float dt_s);
    void handleE4(const SensorData& d, float dt_s);
    void handleE5(const SensorData& d, float dt_s);
    void handleE6(const SensorData& d, float dt_s);

    void logSample(const SensorData& d);
};

extern ExperimentRunner experimentRunner;
