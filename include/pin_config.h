#pragma once

// =====================================================
// pin_config.h - GPIO mapping and I²C topology
// UPDATED: now uses TCA9548A multiplexer
// =====================================================

// I²C bus (ESP32 → TCA9548A → individual channels)
#define PIN_I2C_SDA          21
#define PIN_I2C_SCL          22

// TCA9548A I²C multiplexer
#define ADDR_TCA9548         0x70   // default address (A0=A1=A2=GND)

// TCA channels for each sensor
#define TCA_CH_INA3221       0      // INA3221 on TCA channel 0 (SC0/SD0)
#define TCA_CH_INA219        1      // INA219 on TCA channel 1 (SC1/SD1)
#define TCA_CH_OLED          2      // optional: OLED on channel 2

// Sensor addresses (both on 0x40, but isolated by multiplexer)
#define ADDR_INA3221         0x40
#define ADDR_INA219          0x40   // <-- same as INA3221, but on different TCA channel

// INA3221 channel mapping (which physical pair is used)
// Both PV solar panel AND USB charge connect to IN+1 / IN-1
#define INA3221_CH_CHARGE     1
#define INA3221_CH_AUX2       2     // unused
#define INA3221_CH_AUX3       3     // unused

// Optional GPIOs
#define PIN_BTN_EXPERIMENT   0
#define PIN_LED              2
