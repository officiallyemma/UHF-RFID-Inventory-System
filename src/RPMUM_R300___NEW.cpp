#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <DIYables_OLED_SSD1309.h>
#include <Wire.h>

#include "display_utils.h"
#include "rfid_tags.h"
#include "serial_utils.h"

// SCREEN DEFS
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define SCREEN_SDA 33
#define SCREEN_SCL 32
DIYables_OLED_SSD1309 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// SCAN DEFS
#define RFID_RX 26
#define RFID_TX 14

// UART
HardwareSerial RFID(1);

// RX buffer
uint8_t recvBuffer[128];
uint8_t recvIndex = 0;
bool isHeadDetected = false;

static unsigned long quietUntil = 0;

uint8_t channelLoops = 0;

void displayLoop();
void sendCommand(uint8_t cmd[], size_t cmdSize);
void parseResponse();
void parseTag();
void parseSuccess();
void dumpHex();
int16_t roundRssi(float value);
uint8_t checksum(const uint8_t* data, size_t length);

bool doSerial = true;

// TX buffer
uint8_t inventoryCmd[8];

void setup() {
  Serial.begin(115200);

  // RX = 14, TX = 26
  RFID.begin(115200, SERIAL_8N1, RFID_RX, RFID_TX);

  inventoryCmd[0] = 0xA0;  // head
  inventoryCmd[1] = 0x06;  // len
  inventoryCmd[2] = 0x01;  // reader address
  inventoryCmd[3] = 0x8B;  // cmd (undocumented)
  inventoryCmd[4] = 0x10;  // channel
  inventoryCmd[5] = 0x00;  // tag a/b
  inventoryCmd[6] = 0xFF;  // repeat

  // inventoryCmd[0] = 0xA0;  // head
  // inventoryCmd[1] = 0x04;  // len
  // inventoryCmd[2] = 0x01;  // reader address
  // inventoryCmd[3] = 0x89;  // cmd (undocumented)
  // inventoryCmd[4] = 0xF0;  // channel

  // inventoryCmd[5] = 0x00;  // tag a/b
  // inventoryCmd[6] = 0xA0;  // repeat
  // inventoryCmd[7] = checksum(inventoryCmd, sizeof(inventoryCmd));
  sendCommand(inventoryCmd, sizeof(inventoryCmd));

  // testChannelHopAverages(0x01, 0xFE, 0x02);

  println("RFID started");

  Wire.begin(SCREEN_SDA, SCREEN_SCL);

  if (!display.begin(SSD1309_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    println(F("SSD1309 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.display();
  println("Display init'd");
}

// void printHexByte(uint8_t b) {
//   if (b < 0x10) print("0");
//   print(b, HEX);
// }

// void printHexTwoBytes(uint16_t b) {
//   if (b < 0x1000) print("0");
//   if (b < 0x0100) print("0");
//   if (b < 0x0010) print("0");
//   print(b, HEX);
// }

static unsigned long lastSend = 0;

void loop() {
  // to allow bidirectional communication with the demo software (which contains config options)
  // pipe ESP32 Serial input to the serial channel of the reader
  while (Serial.available()) {
    uint8_t b = Serial.read();
    RFID.write(b);

    // we need to go quiet on both our comms TO the reader board
    quietUntil = millis() + 10000;
    // and FROM our mcu board
    doSerial = false;
  }

  // pipe board's output back out through Serial
  if (millis() < quietUntil) {
    // pipe esp out to serial out
    while (RFID.available()) {
      uint8_t b = RFID.read();
      Serial.write(b);
    }
  }

  // // if we are allowed, send inventory command
  // if (millis() >= quietUntil && millis() - lastSend >= 5000) {
  //   doSerial = true;
  //   lastSend = millis();

  //   sendCommand(inventoryCmd, sizeof(inventoryCmd));

  //   // uint8_t tempCmd[5];
  //   // tempCmd[0] = 0xA0;  // head
  //   // tempCmd[1] = 0x03;  // len
  //   // tempCmd[2] = 0x01;  // reader address
  //   // tempCmd[3] = 0x7B;  // cmd (undocumented)
  //   // sendCommand(tempCmd, sizeof(tempCmd));
  //   // for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
  //   //   const Tag& tag = knownTags[i];
  //   //   if (tag.pings == 0) continue;
  //   //   uint16_t tailId = getTagTailId(tag);
  //   //   print(">TAG_");
  //   //   printHexTwoBytes(tailId);
  //   //   print(":");
  //   //   bool isActive = millis() - tag.lastSeen < 1000;
  //   //   print(isActive ? "true" : "false");
  //   //   println("|tnp");
  //   // }
  // }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 100) {
    lastPrint = millis();
    // prettyPrintKnownTags();
    displayLoop(display, inventoryCmd[4]);
  }

  // read from RFID rx
  // packets vary in length
  while (RFID.available()) {
    uint8_t b = RFID.read();

    // if we have not yet encountered a packet, search for header byte
    if (!isHeadDetected) {
      if (b == 0xA0) {
        isHeadDetected = true;
        recvBuffer[0] = b;
        recvIndex++;
      }
      continue;
    }

    // receive the next byte
    recvBuffer[recvIndex++] = b;

    // reset if we have filled the buffer
    if (recvIndex >= sizeof(recvBuffer)) {
      print("WARN: buffer size less than required by packet.");
      recvIndex = 0;
      isHeadDetected = false;
    }

    // check against expected packet length
    uint8_t len = recvBuffer[1];
    if (recvIndex >= len + 2) {
      parseResponse();
      recvIndex = 0;
      isHeadDetected = false;
    }
  }
}

// take in a pointer to an array
void sendCommand(uint8_t cmd[], size_t cmdSize) {
  cmd[cmdSize - 1] = checksum(cmd, cmdSize);
  RFID.write(cmd, cmdSize);
}

void parseResponse() {
  uint8_t len = recvBuffer[1];
  uint8_t cmd = recvBuffer[3];

  // INVENTORY SUCCESS FRAME
  if (len == 0x0A && cmd == 0x8B) {
    parseSuccess();
    return;
  }

  // TAG FRAME
  if (len == 0x13 && cmd == 0x8B) {
    parseTag();
    return;
  }

  // if (cmd == 0x89) {
  //   parseTag2();
  //   return;
  // }

  // if (cmd == 0x7B) {
  //   // print frame
  //   print("Frame: ");
  //   for (int i = 0; i < recvIndex; i++) {
  //     printHexByte(recvBuffer[i]);
  //     print(" ");
  //   }
  // }

  println("Unknown frame:");
  dumpHex();
}

void parseSuccess() {
  // there is a ramp up (setup overhead) when calling this. serial calls will endure a delay.
  // there is probably a better way to do this
  sendCommand(inventoryCmd, sizeof(inventoryCmd));
}

int16_t roundRssi(float value) { return (int16_t)(value < 0 ? value - 0.5f : value + 0.5f); }

void parseTag() {
  int epcOffset = 7;
  int8_t rssi = (int8_t)recvBuffer[recvIndex - 2];

  if (recvIndex > epcOffset + 2) {
    uint8_t epcLen = recvIndex - 2 - epcOffset;
    uint8_t epc[MaxEpcLength];
    epcLen = normalizeEpc(recvBuffer + epcOffset, epcLen, epc);
    reportTag(epc, epcLen, rssi);

    int16_t smoothedDisplay = roundRssi(getSmoothedRssi(epc, epcLen, rssi));
    uint16_t tailId = 0;
    if (epcLen >= 2) {
      tailId = ((uint16_t)epc[epcLen - 2] << 8) | epc[epcLen - 1];
    }
  }
}

void dumpHex() {
  print("RAW: ");
  for (int i = 0; i < recvIndex; i++) {
    if (recvBuffer[i] < 0x10) print("0");
    print(recvBuffer[i], HEX);
    print(" ");
  }
  println();
}

uint8_t checksum(const uint8_t* data, size_t length) {
  uint8_t sum = 0;

  for (size_t i = 0; i < length - 1; i++) {
    sum += data[i];
  }

  return (uint8_t)(~sum + 1);
}

int8_t rssiToDecibel(uint8_t rssi) {
  //
  return (rssi - 0x62) - 31;
}
