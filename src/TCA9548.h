#pragma once
#include <Arduino.h>

// =====================================================
// TCA9548.h - Simple TCA9548A I²C multiplexer driver
// =====================================================

class TCA9548 {
public:
    bool begin(uint8_t address = 0x70);

    /**
     * Select active channel (0..7) on the multiplexer.
     * Other channels are disconnected.
     * @return true on success, false if I²C communication failed
     */
    bool selectChannel(uint8_t ch);

    /**
     * Disable all channels.
     */
    bool disableAll();

    bool isReady() const { return _ready; }

private:
    uint8_t _addr = 0x70;
    bool    _ready = false;
    int8_t  _lastCh = -1;
};

extern TCA9548 tca;
