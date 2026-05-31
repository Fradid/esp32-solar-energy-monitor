#pragma once
#include <Arduino.h>
#include "SensorManager.h"

class DataExporter {
public:
    static void begin();
    static void startNew(const char* expName);
    static void writeRow(uint32_t t_s, const SensorData& d, float soc,
                          float energy_in_Wh, float energy_out_Wh);
    static void close();
    static String listFiles();
    static String getFilePath();
};
