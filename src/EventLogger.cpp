#include "EventLogger.h"

EventLogger eventLog;

void EventLogger::begin() {
    _head = 0;
    _count = 0;
}

void EventLogger::add(EventType t, const char* msg) {
    EventEntry& e = _entries[_head];
    e.timestamp_ms = millis();
    e.type = t;
    strncpy(e.message, msg, sizeof(e.message) - 1);
    e.message[sizeof(e.message) - 1] = '\0';

    _head = (_head + 1) % MAX_LOG;
    if (_count < MAX_LOG) _count++;

    Serial.printf("[Event] %s: %s\n", typeStr(t), msg);
}

const char* EventLogger::typeStr(EventType t) {
    switch (t) {
        case EVT_BOOT:               return "BOOT";
        case EVT_WIFI_CONNECTED:     return "WIFI_OK";
        case EVT_WIFI_LOST:          return "WIFI_LOST";
        case EVT_EXPERIMENT_START:   return "EXP_START";
        case EVT_EXPERIMENT_STOP:    return "EXP_STOP";
        case EVT_E5_PHASE:           return "E5_PHASE";
        case EVT_SENSOR_ERROR:       return "SENSOR_ERR";
        case EVT_PROTECTION:         return "PROTECT";
        case EVT_TS_PUBLISH_FAIL:    return "TS_FAIL";
        default:                     return "UNKNOWN";
    }
}

String EventLogger::getAsJSON(uint32_t maxEntries) {
    String out = "[";
    int n = min((int)maxEntries, _count);
    int start = (_head - n + MAX_LOG) % MAX_LOG;
    for (int i = 0; i < n; i++) {
        const EventEntry& e = _entries[(start + i) % MAX_LOG];
        if (i > 0) out += ",";
        out += "{\"t\":" + String(e.timestamp_ms);
        out += ",\"type\":\"" + String(typeStr(e.type)) + "\"";
        out += ",\"msg\":\"" + String(e.message) + "\"}";
    }
    out += "]";
    return out;
}

void EventLogger::clear() {
    _head = 0;
    _count = 0;
}
