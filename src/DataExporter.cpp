#include "DataExporter.h"
#include <SPIFFS.h>

static File _file;
static String _currentPath;

void DataExporter::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[DataExporter] SPIFFS mount failed");
        return;
    }
    Serial.printf("[DataExporter] SPIFFS OK, total=%lu used=%lu\n",
                  (unsigned long)SPIFFS.totalBytes(),
                  (unsigned long)SPIFFS.usedBytes());
}

void DataExporter::startNew(const char* expName) {
    if (_file) _file.close();

    // Build filename with timestamp
    String name = "/exp_" + String(expName) + "_" + String(millis() / 1000) + ".csv";
    _currentPath = name;
    _file = SPIFFS.open(name, FILE_WRITE);
    if (!_file) {
        Serial.printf("[DataExporter] FAILED to open %s\n", name.c_str());
        return;
    }

    // CSV header
    _file.println("t_s,V_charge,I_charge_A,V_load,I_load_A,V_bat,SOC_pct,E_in_Wh,E_out_Wh");
    Serial.printf("[DataExporter] Started: %s\n", name.c_str());
}

void DataExporter::writeRow(uint32_t t_s, const SensorData& d, float soc,
                             float energy_in_Wh, float energy_out_Wh) {
    if (!_file) return;
    _file.printf("%lu,%.3f,%.4f,%.3f,%.4f,%.3f,%.2f,%.4f,%.4f",
        (unsigned long)t_s,
        d.V_charge, d.I_charge,
        d.V_load, d.I_load,
        d.V_bat, soc,
        energy_in_Wh, energy_out_Wh);
}

void DataExporter::close() {
    if (_file) {
        _file.flush();
        _file.close();
        Serial.printf("[DataExporter] Closed: %s\n", _currentPath.c_str());
    }
}

String DataExporter::listFiles() {
    String out = "[";
    File root = SPIFFS.open("/");
    if (!root) return "[]";
    File f = root.openNextFile();
    bool first = true;
    while (f) {
        String n = f.name();
        if (n.endsWith(".csv")) {
            if (!first) out += ",";
            out += "{\"name\":\"" + n + "\",\"size\":" + String(f.size()) + "}";
            first = false;
        }
        f = root.openNextFile();
    }
    out += "]";
    return out;
}

String DataExporter::getFilePath() {
    return _currentPath;
}
