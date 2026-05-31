#include "WebServer.h"
#include "SensorManager.h"
#include "SOCEstimator.h"
#include "ExperimentRunner.h"
#include "ThingSpeakPublisher.h"
#include "EventLogger.h"
#include "DataExporter.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

WebServerManager webServer;

static AsyncWebServer _server(80);
static AsyncWebSocket _ws("/ws");

static String buildStateJSON() {
    JsonDocument doc;
    SensorData d = sensors.readAll();
    const auto& exp = experimentRunner.getStatus();

    doc["uptime_s"] = millis() / 1000;
    doc["soc"] = soc.getSOC();
    doc["V_charge"] = d.V_charge;
    doc["I_charge"] = d.I_charge;
    doc["V_load"] = d.V_load;
    doc["I_load"] = d.I_load;
    doc["V_bat"] = d.V_bat;
    doc["sensors_ok"] = d.ina3221_ok && d.ina219_ok;

    JsonObject e = doc["experiment"].to<JsonObject>();
    e["type"] = (int)exp.type;
    e["name"] = experimentRunner.getCurrentName();
    e["running"] = exp.running;
    e["elapsed_s"] = exp.elapsed_s;
    e["energy_in_Wh"] = exp.energy_in_Wh;
    e["energy_out_Wh"] = exp.energy_out_Wh;
    e["startSOC"] = exp.startSOC;
    e["deltaSOC"] = exp.deltaSOC;
    e["samples"] = exp.samples;

    doc["ts_published"] = tsPublisher.getSuccessCount();
    doc["ts_failed"] = tsPublisher.getFailCount();

    String out;
    serializeJson(doc, out);
    return out;
}

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client #%u connected\n", client->id());
        client->text(buildStateJSON());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Client #%u disconnected\n", client->id());
    }
}

void WebServerManager::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[Web] SPIFFS mount failed");
    }

    _ws.onEvent(onWsEvent);
    _server.addHandler(&_ws);

    // Static files
    _server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // === REST API ===

    // GET /api/state — current state
    _server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", buildStateJSON());
    });

    // POST /api/experiment/start?type=1&param=0
    _server.on("/api/experiment/start", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("type", true)) {
            req->send(400, "application/json", "{\"error\":\"missing type\"}");
            return;
        }
        int t = req->getParam("type", true)->value().toInt();
        float p = req->hasParam("param", true)
                  ? req->getParam("param", true)->value().toFloat() : 0;
        bool ok = experimentRunner.start((ExperimentType)t, p);
        req->send(200, "application/json",
                  String("{\"ok\":") + (ok ? "true" : "false") + "}");
    });

    // POST /api/experiment/stop
    _server.on("/api/experiment/stop", HTTP_POST, [](AsyncWebServerRequest* req) {
        experimentRunner.stop();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // POST /api/soc/set?value=50
    _server.on("/api/soc/set", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("value", true)) {
            req->send(400);
            return;
        }
        float v = req->getParam("value", true)->value().toFloat();
        soc.setSOC(v);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // GET /api/log
    _server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", eventLog.getAsJSON(50));
    });

    // GET /api/files - list CSV files
    _server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", DataExporter::listFiles());
    });

    // GET /api/download?file=... — download a CSV file
    _server.on("/api/download", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("file")) {
            req->send(400);
            return;
        }
        String path = req->getParam("file")->value();
        if (!path.startsWith("/")) path = "/" + path;
        if (!SPIFFS.exists(path)) {
            req->send(404);
            return;
        }
        req->send(SPIFFS, path, "text/csv", true);
    });

    _server.begin();
    Serial.println("[Web] Server started on port 80");
}

void WebServerManager::broadcastState() {
    if ((millis() - _lastBroadcast_ms) < 1000) return;
    if (_ws.count() == 0) return;
    _ws.textAll(buildStateJSON());
    _lastBroadcast_ms = millis();
}
