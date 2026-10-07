#pragma once
#include <Arduino.h>

struct TouchPoint {
  uint16_t x;
  uint16_t y;
  uint16_t xRaw;
  uint16_t yRaw;
  uint16_t zRaw;
};

class XPT2046_Bitbang {
 public:
  XPT2046_Bitbang(uint8_t mosi, uint8_t miso, uint8_t clk, uint8_t cs,
                  uint16_t width, uint16_t height);
  void begin();
  TouchPoint getTouch();

 private:
  uint8_t mosi_, miso_, clk_, cs_;
  uint16_t width_, height_;
  void writeSpi(uint8_t command);
  uint16_t readSpi(uint8_t command);
};
