#include "rfid_reader.h"

#include "print.h"
#include "tag_manager.h"

HardwareSerial RFID(1);
uint8_t recvBuffer[128];
uint8_t recvIndex = 0;
bool isHeadDetected = false;
uint8_t inventoryCmd[8];
bool isESPSerialOutActive = true;
unsigned long RFIDCmdsQuietUntil = 0;

void sendCommand(uint8_t cmd[], size_t cmdSize) {
  cmd[cmdSize - 1] = checksum(cmd, cmdSize);
  RFID.write(cmd, cmdSize);
}

uint8_t checksum(const uint8_t* data, size_t length) {
  uint8_t sum = 0;

  for (size_t i = 0; i < length - 1; i++) {
    sum += data[i];
  }

  return (uint8_t)(~sum + 1);
}

void dumpHex() {
  print("RAW: ");
  for (int i = 0; i < recvIndex; i++) {
    if (recvBuffer[i] < 0x10) {
      print("0");
    }
    print(recvBuffer[i], HEX);
    print(" ");
  }
  println();
}

void parseTag() {
  int epcOffset = 7;
  int8_t rssi = (int8_t)recvBuffer[recvIndex - 2];

  if (recvIndex > epcOffset + 2) {
    uint8_t epcLen = recvIndex - 2 - epcOffset;
    uint8_t epc[MaxEpcLength];
    epcLen = normalizeEpc(recvBuffer + epcOffset, epcLen, epc);
    reportTag(epc, epcLen, rssi);
  }
}

void parseSuccess() { sendCommand(inventoryCmd, sizeof(inventoryCmd)); }

void parseResponse() {
  uint8_t len = recvBuffer[1];
  uint8_t cmd = recvBuffer[3];

  if (len == 0x0A && cmd == 0x8B) {
    parseSuccess();
    return;
  }

  if (len == 0x13 && cmd == 0x8B) {
    parseTag();
    return;
  }

  println("Unknown frame:");
  dumpHex();
}

void processRfidInput() {
  while (RFID.available()) {
    uint8_t b = RFID.read();

    if (!isHeadDetected) {
      if (b == 0xA0) {
        isHeadDetected = true;
        recvBuffer[0] = b;
        recvIndex++;
      }
      continue;
    }

    recvBuffer[recvIndex++] = b;

    if (recvIndex >= sizeof(recvBuffer)) {
      print("WARN: buffer size less than required by packet.");
      recvIndex = 0;
      isHeadDetected = false;
    }

    uint8_t len = recvBuffer[1];
    if (recvIndex >= len + 2) {
      parseResponse();
      recvIndex = 0;
      isHeadDetected = false;
    }
  }
}
