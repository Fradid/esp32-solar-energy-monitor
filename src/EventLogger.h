#pragma once
#include <Arduino.h>

enum EventType {
    EVT_BOOT = 0,
    EVT_WIFI_CONNECTED,
    EVT_WIFI_LOST,
    EVT_EXPERIMENT_START,
    EVT_EXPERIMENT_STOP,
    EVT_E5_PHASE,
    EVT_SENSOR_ERROR,
    EVT_PROTECTION,
    EVT_TS_PUBLISH_FAIL,
};

struct EventEntry {
    uint32_t timestamp_ms;
    EventType type;
    char message[64];
};

class EventLogger {
public:
    void begin();
    void add(EventType t, const char* msg);
    String getAsJSON(uint32_t maxEntries = 50);
    void clear();

private:
    static constexpr int MAX_LOG = 100;
    EventEntry _entries[MAX_LOG];
    int _head = 0;
    int _count = 0;

    const char* typeStr(EventType t);
};

extern EventLogger eventLog;
