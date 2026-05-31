#include "ThingSpeakPublisher.h"
#include "../include/constants.h"
#include "../include/secrets.h"
#include <WiFi.h>
#include <ThingSpeak.h>

ThingSpeakPublisher tsPublisher;
static WiFiClient _wifiClient;

void ThingSpeakPublisher::begin() {
    ThingSpeak.begin(_wifiClient);
    Serial.printf("[ThingSpeak] Initialized, channel ID=%lu\n",
                  (unsigned long)THINGSPEAK_CHANNEL_ID);
}

void ThingSpeakPublisher::publishIfReady(const SensorData& d, float socVal, float power) {
    if ((millis() - _lastPublish_ms) < TS_PUBLISH_INTERVAL_MS) return;
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ThingSpeak] WiFi not connected, skip publish");
        return;
    }

    // Field 1: V_charge (from active charge channel)
    float V_chrg = d.V_charge;
    float I_chrg = d.I_charge;

    ThingSpeak.setField(1, V_chrg);
    ThingSpeak.setField(2, I_chrg * 1000.0f);   // mA
    ThingSpeak.setField(3, d.V_load);
    ThingSpeak.setField(4, d.I_load * 1000.0f);   // mA
    ThingSpeak.setField(5, socVal);
    ThingSpeak.setField(6, power);

    int rc = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_WRITE_KEY);
    if (rc == 200) {
        _successCount++;
        Serial.printf("[ThingSpeak] Published #%lu OK\n", (unsigned long)_successCount);
    } else {
        _failCount++;
        Serial.printf("[ThingSpeak] FAIL rc=%d\n", rc);
    }
    _lastPublish_ms = millis();
}
