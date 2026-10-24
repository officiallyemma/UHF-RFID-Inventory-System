#pragma once

#include <Arduino.h>

extern bool isESPSerialOutActive;

template <typename T>
inline void print(const T& value) {
  if (isESPSerialOutActive) {
    Serial.print(value);
  }
}

template <typename T>
inline void print(const T& value, int format) {
  if (isESPSerialOutActive) {
    Serial.print(value, format);
  }
}

template <typename T, typename... Args>
inline void print(const T& first, const Args&... rest) {
  if (isESPSerialOutActive) {
    Serial.print(first);
    print(rest...);
  }
}

template <typename T>
inline void println(const T& value) {
  if (isESPSerialOutActive) {
    Serial.println(value);
  }
}

inline void println() {
  if (isESPSerialOutActive) {
    Serial.println();
  }
}

inline void printHexByte(uint8_t b) {
  if (b < 0x10) print("0");
  print(b, HEX);
}

inline void printTwoHexBytes(uint16_t b) {
  if (b < 0x1000) print("0");
  if (b < 0x0100) print("0");
  if (b < 0x0010) print("0");
  print(b, HEX);
}
