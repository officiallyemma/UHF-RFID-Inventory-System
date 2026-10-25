#pragma once

#include <Arduino.h>
#include <DIYables_OLED_SSD1309.h>

enum TextAlign {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT,
};

void drawJustifiedText(DIYables_OLED_SSD1309& display, const String& text, int16_t y, TextAlign align);
void drawText(DIYables_OLED_SSD1309& display, int x, int y, const String& text);
void drawSpinner(DIYables_OLED_SSD1309& display, uint8_t frame, int16_t cx, int16_t cy, uint8_t radius);
void searchingLoop(DIYables_OLED_SSD1309& display);
void debugDisplayLoop(DIYables_OLED_SSD1309& display);
