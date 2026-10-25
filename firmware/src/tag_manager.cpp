#include "tag_manager.h"

#include <cstring>

#include "tag_config.h"

Tag knownTags[MaxTrackedTags] = {};

int16_t roundRssi(float value) { return (int16_t)(value < 0 ? value - 0.5f : value + 0.5f); }

bool epcEqual(const Tag& tag, const uint8_t* epc, uint8_t epcLen) {
  if (tag.epcLen != epcLen) {
    return false;
  }

  for (uint8_t i = 0; i < epcLen; ++i) {
    if (tag.epc[i] != epc[i]) {
      return false;
    }
  }

  return true;
}

bool epcTailEqual(const uint8_t* epcA, uint8_t lenA, const uint8_t* epcB, uint8_t lenB, uint8_t tailLen) {
  if (lenA < tailLen || lenB < tailLen) {
    return false;
  }

  return memcmp(epcA + lenA - tailLen, epcB + lenB - tailLen, tailLen) == 0;
}

uint16_t getTagTailId(const Tag& tag) {
  if (tag.epcLen < 2) {
    return 0;
  }

  return ((uint16_t)tag.epc[tag.epcLen - 2] << 8) | tag.epc[tag.epcLen - 1];
}

uint8_t normalizeEpc(const uint8_t* epc, uint8_t epcLen, uint8_t* output) {
  uint8_t normalizedLen = epcLen > MaxEpcLength ? MaxEpcLength : epcLen;
  const uint8_t* source = epc;

  if (epcLen > MaxEpcLength) {
    source += epcLen - MaxEpcLength;
  }

  memcpy(output, source, normalizedLen);
  return normalizedLen;
}

float getSmoothedRssi(const uint8_t* epc, uint8_t epcLen, int8_t rawRssi) {
  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    if (epcEqual(knownTags[i], epc, epcLen)) {
      return knownTags[i].smoothedRssi;
    }
  }

  return rawRssi;
}

void reportTag(const uint8_t* epc, uint8_t epcLen, int8_t rssi) {
  unsigned long now = millis();
  uint8_t normalizedEpc[MaxEpcLength];
  uint8_t normalizedLen = normalizeEpc(epc, epcLen, normalizedEpc);

  // try to update already existing tag
  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    if (epcEqual(knownTags[i], normalizedEpc, normalizedLen)) {
      knownTags[i].smoothedRssi = knownTags[i].smoothedRssi * (1.0f - RssiSmoothingAlpha) + rssi * RssiSmoothingAlpha;
      knownTags[i].rssi = rssi;
      knownTags[i].lastSeen = now;
      knownTags[i].pings++;
      return;
    }
  }

  // otherwise look for empty spots
  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    if (knownTags[i].pings == 0) {
      memcpy(knownTags[i].epc, normalizedEpc, normalizedLen);
      knownTags[i].epcLen = normalizedLen;
      knownTags[i].rssi = rssi;
      knownTags[i].smoothedRssi = rssi;
      knownTags[i].lastSeen = now;
      knownTags[i].pings = 1;
      return;
    }
  }

  // if all else fails, find the oldest tag and replace it
  uint8_t oldestIndex = 0;
  unsigned long oldestSeen = knownTags[0].lastSeen;

  for (uint8_t i = 1; i < MaxTrackedTags; ++i) {
    if (knownTags[i].lastSeen < oldestSeen) {
      oldestSeen = knownTags[i].lastSeen;
      oldestIndex = i;
    }
  }

  memcpy(knownTags[oldestIndex].epc, normalizedEpc, normalizedLen);
  knownTags[oldestIndex].epcLen = normalizedLen;
  knownTags[oldestIndex].rssi = rssi;
  knownTags[oldestIndex].smoothedRssi = rssi;
  knownTags[oldestIndex].lastSeen = now;
  knownTags[oldestIndex].pings = 1;
}

void prettyPrintKnownTags() {
  Serial.println("KNOWN TAGS:");
  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    const Tag& tag = knownTags[i];
    if (tag.epcLen == 0 || millis() - tag.lastSeen > 10000) {
      continue;
    }

    uint16_t tailId = getTagTailId(tag);
    Serial.print("  [");
    if (tailId < 0x1000) Serial.print("0");
    if (tailId < 0x0100) Serial.print("0");
    if (tailId < 0x0010) Serial.print("0");
    Serial.print(tailId, HEX);
    Serial.print("] ");
    Serial.print("RSSI=");
    Serial.print(tag.rssi);
    Serial.print("  smoothed=");
    Serial.print(tag.smoothedRssi, 1);
    Serial.print("  pings=");
    Serial.print(tag.pings);
    Serial.print("  ageMs=");
    Serial.println(millis() - tag.lastSeen);
  }
}
