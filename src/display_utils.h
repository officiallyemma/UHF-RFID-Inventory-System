#pragma once

#include <Arduino.h>
#include <DIYables_OLED_SSD1309.h>

#include "rfid_tags.h"

enum TextAlign { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

void drawJustifiedText(DIYables_OLED_SSD1309& display, const String& text, int16_t y, TextAlign align);
void displayLoop(DIYables_OLED_SSD1309& display, uint8_t inventoryChannel);
