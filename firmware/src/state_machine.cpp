#include "state_machine.h"

#include <Arduino.h>

#include "display_ui.h"
#include "print.h"
#include "rfid_reader.h"
#include "tag_config.h"
#include "tag_manager.h"

extern DIYables_OLED_SSD1309 display;
SuccessState successState;
FailState failState;

void State::enter() {}
void State::loop() {}
void State::exit() {}

// create a state machine
StateMachine::StateMachine() : current(nullptr) {}

// begin by entering an initial state
void StateMachine::begin(State* initial) {
  current = initial;
  if (current) {
    current->enter();
  }
}

void SearchingState::enter() {
  println("Entering Searching State");

  // count up saved tags
  savedTagsTotal = 0;
  for (uint8_t savedIndex = 0; savedIndex < maxSavedTags; savedIndex++) {
    if (savedTags[savedIndex].epcLen == 0) continue;
    savedTagsTotal++;
  }

  // for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
  //   if (knownTags[i].pings == 0) continue;
  //   if (knownTags[i].epcLen == 0) continue;
  //   uint16_t tailId = getTagTailId(knownTags[i]);

  // }
}

void SearchingState::loop() {
  processRfidInput();

  int savedTagsSeen = 0;

  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    if (knownTags[i].pings == 0) continue;
    if (knownTags[i].epcLen == 0) continue;
    for (uint8_t savedIndex = 0; savedIndex < maxSavedTags; savedIndex++) {
      if (savedTags[savedIndex].epcLen == 0) continue;
      if (epcTailEqual(knownTags[i].epc, knownTags[i].epcLen, savedTags[savedIndex].epc, savedTags[savedIndex].epcLen, 2)) {
        savedTagsSeen++;
      }
    }
  }

  if (savedTagsSeen == savedTagsTotal) {
    return machine->change(&successState);
  }

  if (millis() > 5 * 60 * 1000) {
    if (savedTagsSeen < savedTagsTotal) {
      println("Timeout reached, not all tags seen, going to sleep");
      return machine->change(&failState);
    }
  }

  static unsigned long lastSend = 0;
  if (millis() - lastSend >= (1000 / 60)) {
    lastSend = millis();

    display.clearDisplay();

    uint8_t spinnerFrame = (millis() / 120) % 8;
    drawSpinner(display, spinnerFrame, 64, 22, 18);

    display.drawLine((128 / 2) - 10, 20 + 5, (128 / 2) + 10, 22 - 5, SSD1309_PIXEL_ON);

    if (savedTagsSeen >= 9) {
      drawText(display, (128 / 2) - 2 - 5, 22 - 11, String(savedTagsSeen));

    } else {
      drawText(display, (128 / 2) - 2, 22 - 11, String(savedTagsSeen));
    }

    if (savedTagsTotal >= 9) {
      drawText(display, (128 / 2) - 2, 22 + 3, String(savedTagsTotal));

    } else {
      drawText(display, (128 / 2) - 2, 22 + 5, String(savedTagsTotal));
    }

    //  print each saved tag that has NOT been seen yet to the screen
    String str1 = "";
    String str2 = "";
    for (uint8_t savedIndex = 0; savedIndex < maxSavedTags; savedIndex++) {
      if (savedTags[savedIndex].epcLen == 0) continue;

      bool tagSeen = false;
      for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
        if (knownTags[i].pings == 0) continue;
        if (knownTags[i].epcLen == 0) continue;
        if (epcTailEqual(knownTags[i].epc, knownTags[i].epcLen, savedTags[savedIndex].epc, savedTags[savedIndex].epcLen, 2)) {
          tagSeen = true;
          break;
        }
      }

      if (!tagSeen) {
        if (str1.length() + savedTags[savedIndex].name.length() > 21) {
          str2 = str2 + ", " + String(savedTags[savedIndex].name);
        } else {
          str1 = str1 + ", " + String(savedTags[savedIndex].name);
        }
        // str = str + ", " + String(savedTags[savedIndex].name);
      }
    }

    // remove trailing comma and space
    if (str1.length() > 0) {
      str1 = str1.substring(2);
      drawJustifiedText(display, str1, 22 + 25, ALIGN_CENTER);
    }

    if (str2.length() > 0) {
      str2 = str2.substring(2);
      drawJustifiedText(display, str2, 22 + 25 + 3 + 7, ALIGN_CENTER);
    }

    display.display();
  }
}

void SearchingState::exit() { println("Leaving Searching State"); }

// // // // // //

void SuccessState::enter() {
  // simple centered bitmap (32x32) - monochrome, 8 pixels per byte, columns then rows
  static const unsigned char logoBitmap[] PROGMEM = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
      0x00, 0x70, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x0f, 0x00,
      0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x03,
      0xe0, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x0f, 0xc0, 0x00, 0x60, 0x0f, 0x80, 0x00, 0x38, 0x1f, 0x00, 0x00, 0x1e, 0x3f, 0x00, 0x00,
      0x0f, 0xfe, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x00, 0x03, 0xf8, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x00, 0x00, 0xf0,
      0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  const int w = 32;
  const int h = 32;
  const int x = (128 - w) / 2;
  const int y = (64 - h) / 2;

  display.clearDisplay();
  display.drawBitmap(x, y, logoBitmap, w, h, SSD1309_PIXEL_ON);
  display.display();

  esp_deep_sleep_start();
}

void SuccessState::exit() { println("Exiting Idle state"); }

void SuccessState::loop() { println("failed to deep sleep"); }

// // // // // //

void FailState::enter() {
  // simple centered bitmap (32x32) - monochrome, 8 pixels per byte, columns then rows
  static const unsigned char logoBitmap[] PROGMEM = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x0f, 0xc0, 0x00, 0x00,
      0x1f, 0xc0, 0x0f, 0x80, 0x3f, 0xc0, 0x0f, 0xf0, 0x7f, 0xc0, 0x0f, 0xf8, 0xff, 0x80, 0x07, 0xfc, 0xff, 0x80, 0x03, 0xff, 0xfe, 0x00,
      0x03, 0xff, 0xfe, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 0x7f,
      0xfc, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x01, 0xff, 0xff, 0x00, 0x01, 0xff, 0xff, 0x80, 0x03, 0xfc, 0xff, 0x80, 0x07, 0xf8, 0x7f, 0xc0,
      0x0f, 0xf0, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xf0, 0x1f, 0xc0, 0x0f, 0xf0, 0x3f, 0x80, 0x03, 0xf8, 0x1f, 0x00, 0x01, 0xf8, 0x1e, 0x00,
      0x00, 0xfc, 0x0c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  const int w = 32;
  const int h = 32;
  const int x = (128 - w) / 2;
  const int y = (64 - h) / 2;

  display.clearDisplay();
  display.drawBitmap(x, y, logoBitmap, w, h, SSD1309_PIXEL_ON);
  display.display();

  esp_deep_sleep_start();
}

void FailState::exit() { println("Exiting Fail state"); }

void FailState::loop() { println("failed to deep sleep"); }
// // // // // //

// handle state lifecycle (enter, loop, exit)
void StateMachine::change(State* next) {
  if (current) {
    current->exit();
  }

  current = next;
  if (current) {
    current->enter();
  }
}

void StateMachine::loop() {
  if (current) {
    current->loop();
    // processRfidInput();
    // debugDisplayLoop(display);
  }
}