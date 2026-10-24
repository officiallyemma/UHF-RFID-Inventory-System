#pragma once

#include <Arduino.h>

// Display configuration
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;
constexpr uint8_t SCREEN_ADDRESS = 0x3C;
constexpr uint8_t SCREEN_SDA = 33;
constexpr uint8_t SCREEN_SCL = 32;

// RFID configuration
constexpr uint8_t RFID_RX = 26;
constexpr uint8_t RFID_TX = 14;
