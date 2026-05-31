#include "TCA9548.h"
#include <Wire.h>

TCA9548 tca;

bool TCA9548::begin(uint8_t address) {
    _addr = address;
    Wire.beginTransmission(_addr);
    _ready = (Wire.endTransmission() == 0);
    _lastCh = -1;
    Serial.printf("[TCA9548] @ 0x%02X: %s\n", _addr, _ready ? "OK" : "NOT FOUND");
    return _ready;
}

bool TCA9548::selectChannel(uint8_t ch) {
    if (!_ready) return false;
    if (ch > 7) return false;
    if (ch == _lastCh) return true;  // already selected, skip

    Wire.beginTransmission(_addr);
    Wire.write(1 << ch);
    int rc = Wire.endTransmission();
    if (rc == 0) {
        _lastCh = ch;
        delayMicroseconds(100);  // settle time
        return true;
    } else {
        Serial.printf("[TCA9548] selectChannel(%d) failed, rc=%d\n", ch, rc);
        return false;
    }
}

bool TCA9548::disableAll() {
    if (!_ready) return false;
    Wire.beginTransmission(_addr);
    Wire.write(0);
    int rc = Wire.endTransmission();
    if (rc == 0) {
        _lastCh = -1;
        return true;
    }
    return false;
}
