#pragma once

#include <Arduino.h>

extern HardwareSerial RFID;
extern uint8_t recvBuffer[128];
extern uint8_t recvIndex;
extern bool isHeadDetected;
extern uint8_t inventoryCmd[8];
extern bool isESPSerialOutActive;
extern unsigned long RFIDCmdsQuietUntil;

void sendCommand(uint8_t cmd[], size_t cmdSize);
uint8_t checksum(const uint8_t* data, size_t length);
void parseResponse();
void processRfidInput();
