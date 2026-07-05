#include "display_utils.h"

#include <algorithm>

#include "serial_utils.h"

void drawJustifiedText(DIYables_OLED_SSD1309& display, const String& text, int16_t y, TextAlign align) {
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

void displayLoop(DIYables_OLED_SSD1309& display, uint8_t inventoryChannel) {
  float rssiSum = 0;
  uint8_t rssiTotalCount = 0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1309_PIXEL_ON);

  int dataSize = sizeof(knownTags) / sizeof(knownTags[0]);
  std::sort(knownTags, knownTags + dataSize, [](const Tag& a, const Tag& b) { return getTagTailId(a) < getTagTailId(b); });

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

    for (uint8_t savedIndex = 0; savedIndex < 10; savedIndex++) {
      if (savedTags[savedIndex].epcLen == 0) continue;

      if (epcTailEqual(knownTags[i].epc, knownTags[i].epcLen, savedTags[savedIndex].epc, savedTags[savedIndex].epcLen, 2)) {
        id = savedTags[savedIndex].name;
        String tag = id + (millis() - knownTags[i].lastSeen > 1000 ? " X " : " " + String(knownTags[i].smoothedRssi, 0));
        rssiSum += knownTags[i].smoothedRssi;
        rssiTotalCount++;
        drawJustifiedText(display, tag, y, align);
        y += 7;
        if (y >= 63) {
          y = 0;
          align = ALIGN_RIGHT;
        }
      }
    }
  }

  drawJustifiedText(display, String(rssiSum / rssiTotalCount), 57, ALIGN_RIGHT);
  drawJustifiedText(display, String(inventoryChannel), 57, ALIGN_CENTER);
  display.display();
}
