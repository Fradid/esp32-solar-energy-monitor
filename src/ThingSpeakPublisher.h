#pragma once
#include <Arduino.h>
#include "SensorManager.h"

class ThingSpeakPublisher {
public:
    void begin();
    void publishIfReady(const SensorData& d, float soc, float power);
    uint32_t getLastPublish_ms() const { return _lastPublish_ms; }
    uint32_t getSuccessCount() const { return _successCount; }
    uint32_t getFailCount() const { return _failCount; }

private:
    uint32_t _lastPublish_ms = 0;
    uint32_t _successCount = 0;
    uint32_t _failCount = 0;
};

extern ThingSpeakPublisher tsPublisher;
