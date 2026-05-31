#include <Arduino.h>
#include <WiFi.h>

#include "SensorManager.h"
#include "SOCEstimator.h"
#include "ExperimentRunner.h"
#include "WebServer.h"
#include "ThingSpeakPublisher.h"
#include "EventLogger.h"
#include "DataExporter.h"

#include "../include/pin_config.h"
#include "../include/constants.h"
#include "../include/secrets.h"

// =====================================================
// main.cpp — Battery Experiments firmware entry point
// =====================================================

TaskHandle_t monitorTaskHandle = nullptr;
TaskHandle_t networkTaskHandle = nullptr;

// -----------------------------------------------------
// Monitor task — Core 1 (sensors, SOC, experiment logic)
// -----------------------------------------------------
void monitorTask(void* /*pv*/) {
    Serial.println("[Monitor] Started on Core 1");
    TickType_t lastWake = xTaskGetTickCount();
    uint32_t lastTick = millis();

    for (;;) {
        SensorData d = sensors.readAll();
        uint32_t now = millis();
        float dt_s = (now - lastTick) / 1000.0f;
        if (dt_s < 0.1f) dt_s = 1.0f;  // safety
        lastTick = now;

        // Update SOC estimate
        // INA3221 channel 1 = active charge source (PV or USB via bridge)
        soc.update(d.I_charge, d.I_load, d.V_bat, dt_s);

        // Tick experiment if running
        experimentRunner.tick(d, dt_s);

        // Sleep until next 1-sec tick
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

// -----------------------------------------------------
// Network task — Core 0 (WiFi, ThingSpeak, WebSocket)
// -----------------------------------------------------
void networkTask(void* /*pv*/) {
    Serial.println("[Network] Started on Core 0");

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[Network] WiFi disconnected, attempting reconnect");
            WiFi.reconnect();
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Broadcast WS state
        webServer.broadcastState();

        // Publish to ThingSpeak (rate-limited internally)
        SensorData d = sensors.readAll();
        float P_now = d.V_bat * (d.I_charge - d.I_load);
        tsPublisher.publishIfReady(d, soc.getSOC(), P_now);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// -----------------------------------------------------
// Setup — runs once on boot
// -----------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n=================================");
    Serial.println("ESP32 Battery Experiments — boot");
    Serial.println("=================================");

    // 1. SPIFFS
    DataExporter::begin();

    // 2. Event log
    eventLog.begin();
    eventLog.add(EVT_BOOT, "System started");

    // 3. Sensors
    if (!sensors.begin()) {
        Serial.println("[Setup] Some sensors not detected — continuing with available");
        eventLog.add(EVT_SENSOR_ERROR, "Sensor detection partial");
    }

    // 4. SOC estimator
    soc.begin(50.0f);  // start at 50%, will be corrected later

    // 5. Experiment runner
    experimentRunner.begin();

    // 6. WiFi
    Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connected, IP=%s\n", WiFi.localIP().toString().c_str());
        eventLog.add(EVT_WIFI_CONNECTED, WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WiFi] Connection failed (continuing offline)");
    }

    // 7. ThingSpeak
    tsPublisher.begin();

    // 8. Web server
    webServer.begin();

    // 9. Create FreeRTOS tasks
    xTaskCreatePinnedToCore(monitorTask, "Monitor", 8192, nullptr,
                             2, &monitorTaskHandle, 1);  // Core 1
    xTaskCreatePinnedToCore(networkTask, "Network", 16384, nullptr,
                             1, &networkTaskHandle, 0);  // Core 0

    Serial.println("[Setup] Complete. System running.");
    Serial.println("Open web UI at: http://" + WiFi.localIP().toString());
}

void loop() {
    // All work happens in FreeRTOS tasks
    delay(1000);
}
