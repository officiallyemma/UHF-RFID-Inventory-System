#include <Arduino.h>
#include <DIYables_OLED_SSD1309.h>
#include <Wire.h>

#include "config.h"
#include "display_ui.h"
#include "print.h"
#include "rfid_reader.h"
#include "state_machine.h"
#include "tag_manager.h"

DIYables_OLED_SSD1309 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
StateMachine machine;
SearchingState searchingState;

void setup() {
  Serial.begin(115200);

  RFID.begin(115200, SERIAL_8N1, RFID_RX, RFID_TX);

  inventoryCmd[0] = 0xA0;
  inventoryCmd[1] = 0x06;
  inventoryCmd[2] = 0x01;
  inventoryCmd[3] = 0x8B;
  inventoryCmd[4] = 0x10;
  inventoryCmd[5] = 0x00;
  inventoryCmd[6] = 0xFF;

  sendCommand(inventoryCmd, sizeof(inventoryCmd));

  Wire.begin(SCREEN_SDA, SCREEN_SCL);
  if (!display.begin(SSD1309_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    println(F("SSD1309 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.display();

  machine.begin(&searchingState);
  searchingState.setMachine(&machine);
}

void loop() {
  while (Serial.available()) {
    uint8_t b = Serial.read();
    RFID.write(b);

    RFIDCmdsQuietUntil = millis() + 10000;
    isESPSerialOutActive = false;
  }

  if (millis() < RFIDCmdsQuietUntil) {
    while (RFID.available()) {
      uint8_t b = RFID.read();
      Serial.write(b);
    }
  }

  machine.loop();
}
