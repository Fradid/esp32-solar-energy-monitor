# Wiring Guide — TCA9548A + INA3221 + INA219

## Topology

```
[USB power 5V]
      │
      ↓
[ESP32 DevKit V1] ───── 3V3 OUT (powers all sensors)
      │                          │
      ├── GPIO21 (SDA) ──┐       ├──→ VCC TCA9548A
      │                  ├──→ SDA TCA9548A
      ├── GPIO22 (SCL) ──┘       ├──→ VCC INA3221
      │                          ├──→ VCC INA219
      └── GND ───────────────────┴──→ (common GND)

[TCA9548A multiplexer]
      │
      ├── SC0/SD0 ───→ SCL/SDA INA3221
      ├── SC1/SD1 ───→ SCL/SDA INA219
      └── A0,A1,A2 ───→ GND (sets TCA address to 0x70)
      └── RESET    ───→ 3V3 (or via 10k pull-up)


--- Power chain (separate from I²C chain) ---

[USB 5V source]      OR      [Solar panel USB out]
       │                              │
       └──────────────┬───────────────┘
                      │
                      ↓
              INA3221 IN+1  ──[shunt]── IN-1
                                          │
                                          ↓
                                    Battery (+)
                                          │
                                          ↓
                              INA219 VIN+ ──[shunt]── VIN-
                                                       │
                                                       ↓
                                                  DC-DC IN+
                                                       │
                                                       ↓
                                                 USB output → fan / load
                                                       │
                                                       ↓
                                                      GND
                                                       │
                                                       └─── common GND
                                                            (Bat-, ESP32 GND, sensor GND)
```

## I²C addresses

| Device     | Address | Where |
|------------|---------|-------|
| TCA9548A   | 0x70    | Main bus (ESP32) |
| INA3221    | 0x40    | TCA channel 0 (SC0/SD0) |
| INA219     | 0x40    | TCA channel 1 (SC1/SD1) |
| OLED       | 0x3C    | TCA channel 2 (optional) |

Both INAs use the same factory address 0x40 — they are isolated by the
multiplexer, so no jumper soldering is required.

## ESP32 pin map

| ESP32 pin  | Connect to                                     |
|------------|------------------------------------------------|
| GPIO 21    | TCA9548A SDA (main bus)                        |
| GPIO 22    | TCA9548A SCL (main bus)                        |
| 3V3        | TCA9548A VCC + INA3221 VCC + INA219 VCC        |
| GND        | TCA9548A GND + all sensor GND + battery (-) + DC-DC GND |
| VIN (5V)   | Powers ESP32 itself (separately from circuit)  |

## Power isolation note

ESP32 powers itself via its own USB-C / micro-USB port (from computer or
external 5V PSU). It does NOT take power from the experimental circuit
under test. This isolation ensures sensor measurements are not skewed
by the microcontroller's own current consumption.

## What is NOT in this setup anymore

- **ESP8266 bridge** — removed. USB charge source connects directly to INA3221.
- **External pull-up resistors** — the TCA9548A breakout has its own pull-ups
  on each channel, so no extras needed.
