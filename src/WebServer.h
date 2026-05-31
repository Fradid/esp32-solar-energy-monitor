#pragma once
#include <Arduino.h>

class WebServerManager {
public:
    void begin();
    void broadcastState();
private:
    uint32_t _lastBroadcast_ms = 0;
};

extern WebServerManager webServer;
