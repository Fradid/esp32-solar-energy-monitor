#pragma once

// =====================================================
// constants.h - System-wide constants
// =====================================================

// Battery (Li-Ion 18650, single cell)
constexpr float Q_NOM_MAH        = 3200.0f;            // mAh, nominal capacity
constexpr float Q_NOM_AS         = Q_NOM_MAH * 3.6f;   // A·s = 11520 (Q in coulombs)
constexpr float U_MAX            = 4.2f;               // V, max charge voltage
constexpr float U_MIN            = 3.0f;               // V, discharge cutoff
constexpr float U_NOMINAL        = 3.7f;               // V, nominal

// Coulomb counting
constexpr float ETA_CHARGE       = 0.95f;              // Charge efficiency (Li-Ion)
constexpr float ETA_DISCHARGE    = 1.00f;              // Discharge efficiency
constexpr float DT_SEC           = 1.0f;               // Sample period (sec)

// SOC thresholds
constexpr float SOC_CRITICAL     = 10.0f;              // %, enter PROTECTION
constexpr float SOC_RECOVER      = 15.0f;              // %, exit PROTECTION (hysteresis)
constexpr float SOC_TARGET       = 95.0f;              // %, charge target

// OCV correction (activates when idle)
constexpr float I_IDLE_THR_A     = 0.05f;              // A
constexpr uint32_t IDLE_TOUT_MS  = 30 * 60 * 1000;     // 30 min before OCV correction
constexpr float OCV_WEIGHT       = 0.3f;               // weight of OCV-based SOC

// ThingSpeak
constexpr uint32_t TS_PUBLISH_INTERVAL_MS = 15000;     // min allowed by free tier

// Sensor sampling
constexpr uint32_t SENSOR_PERIOD_MS = 1000;

// WiFi
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;

// I²C
constexpr uint32_t I2C_FREQUENCY = 100000;             // 100 kHz, safe for all
