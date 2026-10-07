#include "XPT2046_Bitbang.h"

namespace {
constexpr uint8_t READ_X = 0x91;
constexpr uint8_t READ_Y = 0xD1;
constexpr uint8_t READ_Z1 = 0xB1;
constexpr uint8_t READ_Z2 = 0xC1;
constexpr uint8_t CLOCK_DELAY_US = 5;
}

XPT2046_Bitbang::XPT2046_Bitbang(uint8_t mosi, uint8_t miso, uint8_t clk,
                                 uint8_t cs, uint16_t width, uint16_t height)
    : mosi_(mosi), miso_(miso), clk_(clk), cs_(cs), width_(width), height_(height) {}

void XPT2046_Bitbang::begin() {
  pinMode(mosi_, OUTPUT);
  pinMode(miso_, INPUT);
  pinMode(clk_, OUTPUT);
  pinMode(cs_, OUTPUT);
  digitalWrite(cs_, HIGH);
  digitalWrite(clk_, LOW);
}

void XPT2046_Bitbang::writeSpi(uint8_t command) {
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(mosi_, command & (1 << bit));
    digitalWrite(clk_, LOW);
    delayMicroseconds(CLOCK_DELAY_US);
    digitalWrite(clk_, HIGH);
    delayMicroseconds(CLOCK_DELAY_US);
  }
  digitalWrite(mosi_, LOW);
  digitalWrite(clk_, LOW);
}

uint16_t XPT2046_Bitbang::readSpi(uint8_t command) {
  writeSpi(command);
  uint16_t result = 0;
  for (int bit = 15; bit >= 0; --bit) {
    digitalWrite(clk_, HIGH);
    delayMicroseconds(CLOCK_DELAY_US);
    digitalWrite(clk_, LOW);
    delayMicroseconds(CLOCK_DELAY_US);
    result |= static_cast<uint16_t>(digitalRead(miso_)) << bit;
  }
  return result >> 4;
}

TouchPoint XPT2046_Bitbang::getTouch() {
  digitalWrite(cs_, LOW);
  uint16_t z1 = readSpi(READ_Z1);
  uint16_t z = static_cast<uint16_t>(z1 + 4095);
  uint16_t z2 = readSpi(READ_Z2);
  z = static_cast<uint16_t>(z - z2);
  if (z < 100) {
    digitalWrite(cs_, HIGH);
    return {0, 0, 0, 0, 0};
  }
  uint16_t xRaw = readSpi(READ_X);
  uint16_t yRaw = readSpi(READ_Y & ~1U);
  digitalWrite(cs_, HIGH);
  uint16_t x = width_ - 1 - map(xRaw, 0, 4095, 0, width_ - 1);
  uint16_t y = height_ - 1 - map(yRaw, 0, 4095, 0, height_ - 1);
  return {static_cast<uint16_t>(constrain(x, 0, width_ - 1)),
          static_cast<uint16_t>(constrain(y, 0, height_ - 1)), xRaw, yRaw, z};
}
