#pragma once
#include <HyperDisplay_SSD1309.h>

class UG2856KLBAG01_I2C : public SSD1309_Arduino_I2C {
 public:
  UG2856KLBAG01_I2C() : SSD1309_Arduino_I2C(128, 64) {}

  SSD1309_Status_t begin(TwoWire& wire, bool sa0, uint8_t rst) {
    _i2c = &wire;
    _sa0val = sa0;
    _sa0 = sa0 ? 1 : 0;
    _rst = rst;
    return SSD1309_Nominal;
  }
};