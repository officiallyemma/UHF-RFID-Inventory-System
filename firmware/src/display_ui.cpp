#include "display_ui.h"

#include <Adafruit_GFX.h>

#include <cmath>

#include "tag_config.h"
#include "tag_manager.h"

void drawJustifiedText(DIYables_OLED_SSD1309& display, const String& text, int16_t y, TextAlign align) {
  display.setTextSize(1);
  display.setTextColor(SSD1309_PIXEL_ON);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);

  int16_t x;

  switch (align) {
    case ALIGN_LEFT:
      x = 0;
      break;

    case ALIGN_CENTER:
      x = (display.width() - w) / 2;
      break;

    case ALIGN_RIGHT:
      x = display.width() - w;
      break;
  }

  display.setCursor(x, y);
  display.print(text);
}

void drawText(DIYables_OLED_SSD1309& display, int x, int y, const String& text) {
  display.setTextSize(1);
  display.setTextColor(SSD1309_PIXEL_ON);
  display.setCursor(x, y);
  display.print(text);
}

void drawSpinner(DIYables_OLED_SSD1309& display, uint8_t frame, int16_t cx, int16_t cy, uint8_t radius) {
  const uint8_t N = 8;
  int16_t pad = 3;
  display.fillRect(cx - radius - pad, cy - radius - pad, (radius + pad) * 2 + 1, (radius + pad) * 2 + 1, SSD1309_PIXEL_OFF);

  for (uint8_t i = 0; i < N; ++i) {
    float angle = 2.0f * PI * (float)i / (float)N;
    int16_t x = cx + (int16_t)round(cos(angle) * radius);
    int16_t y = cy + (int16_t)round(sin(angle) * radius);

    uint8_t dotR = (i == frame) ? 1 : 3;
    display.fillCircle(x, y, dotR, SSD1309_PIXEL_ON);
  }
}

// void searchingLoop(DIYables_OLED_SSD1309& display) {
// display.clearDisplay();

// uint8_t spinnerFrame = (millis() / 120) % 8;
// drawSpinner(display, spinnerFrame, 64, 22, 18);

// display.drawLine((128 / 2) - 10, 20 + 5, (128 / 2) + 10, 22 - 5, SSD1309_PIXEL_ON);

// int savedTagsSeen = 0;
// int savedTagsTotal = 0;

// if (savedTagsSeen >= 9) {
//   drawText(display, (128 / 2) - 2 - 5, 22 - 11, String(savedTagsSeen));

// } else {
//   drawText(display, (128 / 2) - 2, 22 - 11, String(savedTagsSeen));
// }

// if (savedTagsTotal >= 9) {
//   drawText(display, (128 / 2) - 2, 22 + 3, String(savedTagsTotal));

// } else {
//   drawText(display, (128 / 2) - 2, 22 + 5, String(savedTagsTotal));
// }

// display.display();
// }v

void debugDisplayLoop(DIYables_OLED_SSD1309& display) {
  float rssiSum = 0;
  uint8_t rssiTotalCount = 0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1309_PIXEL_ON);

  int dataSize = sizeof(knownTags) / sizeof(knownTags[0]);
  std::sort(knownTags, knownTags + dataSize, [](const Tag& a, const Tag& b) { return getTagTailId(a) < getTagTailId(b); });

  unsigned long elapsedSeconds = millis() / 1000;
  if (elapsedSeconds == 0) {
    elapsedSeconds = 1;
  }

  float maxPingsPerSec = 0.0f;
  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    knownTags[i].pingsPerSec = knownTags[i].pings > 0 ? (float)knownTags[i].pings / 10 : 0.0f;
    if (knownTags[i].pingsPerSec > maxPingsPerSec) {
      maxPingsPerSec = knownTags[i].pingsPerSec;
    }
  }

  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    knownTags[i].pingsPerSecNormalized = maxPingsPerSec > 0.0f ? knownTags[i].pingsPerSec / maxPingsPerSec : 0.0f;
  }

  TextAlign align = ALIGN_LEFT;
  uint8_t y = 0;
  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    if (knownTags[i].pings == 0) continue;
    if (knownTags[i].epcLen == 0) continue;
    uint16_t tailId = getTagTailId(knownTags[i]);
    String id = "";
    if (tailId < 0x1000) id += "0";
    if (tailId < 0x0100) id += "0";
    if (tailId < 0x0010) id += "0";
    id += String(tailId, HEX);

    // for (uint8_t savedIndex = 0; savedIndex < maxSavedTags; savedIndex++) {
    // if (savedTags[savedIndex].epcLen == 0) continue;

    // if (epcTailEqual(knownTags[i].epc, knownTags[i].epcLen, savedTags[savedIndex].epc, savedTags[savedIndex].epcLen, 2)) {
    // id = savedTags[savedIndex].name;
    String tag = id + ((millis() - knownTags[i].lastSeen > 1000) ? String(" X ") : (String(" ") + String(knownTags[i].pingsPerSecNormalized, 2)));
    rssiSum += knownTags[i].smoothedRssi;
    rssiTotalCount++;
    drawJustifiedText(display, tag, y, align);
    y += 7;
    if (y >= 63) {
      y = 0;
      align = ALIGN_RIGHT;
    }
    // }
    // }
  }

  uint8_t spinnerFrame = (millis() / 120) % 8;
  drawSpinner(display, spinnerFrame, 120, 56, 4);

  display.display();
}
