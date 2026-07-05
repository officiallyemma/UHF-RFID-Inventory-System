#pragma once

#include <Arduino.h>

constexpr uint8_t MaxEpcLength = 24;
constexpr uint8_t MaxTrackedTags = 16;
constexpr float RssiSmoothingAlpha = 0.2f;

struct Tag {
  uint8_t epc[MaxEpcLength];
  uint8_t epcLen;
  int rssi;
  float smoothedRssi;
  unsigned long lastSeen;
  int pings;
};

struct SavedTag {
  uint8_t epc[MaxEpcLength];
  uint8_t epcLen;
  String name;
};

extern Tag knownTags[MaxTrackedTags];
extern SavedTag savedTags[];
extern const uint8_t maxSavedTags;

bool epcEqual(const Tag& tag, const uint8_t* epc, uint8_t epcLen);
bool epcTailEqual(const uint8_t* epcA, uint8_t lenA, const uint8_t* epcB, uint8_t lenB, uint8_t tailLen);
uint16_t getTagTailId(const Tag& tag);
uint8_t normalizeEpc(const uint8_t* epc, uint8_t epcLen, uint8_t* output);
float getSmoothedRssi(const uint8_t* epc, uint8_t epcLen, int8_t rawRssi);
void reportTag(const uint8_t* epc, uint8_t epcLen, int8_t rssi);
void prettyPrintKnownTags();
