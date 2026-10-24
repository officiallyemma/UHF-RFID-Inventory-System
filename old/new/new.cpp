#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <DIYables_OLED_SSD1309.h>
#include <Wire.h>
#include <print.h>

#include <algorithm>
#include <cmath>

// OLED DISPLAY DEFS
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define SCREEN_SDA 33
#define SCREEN_SCL 32
DIYables_OLED_SSD1309 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// RFID READER DEFS
#define RFID_RX 26
#define RFID_TX 14

// we communicate with the reader via UART
HardwareSerial RFID(1);

// RX buffer
uint8_t recvBuffer[128];
uint8_t recvIndex = 0;
bool isHeadDetected = false;

// TX buffer (remains static)
uint8_t inventoryCmd[8];

bool isESPSerialOutActive = true;
static unsigned long RFIDCmdsQuietUntil = 0;
//
//
//
//
void sendCommand(uint8_t cmd[], size_t cmdSize);
uint8_t checksum(const uint8_t* data, size_t length);
void displayLoop(DIYables_OLED_SSD1309& display);
void parseResponse();
//
//
//
//

// //////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);

  // start up the RFID reader
  RFID.begin(115200, SERIAL_8N1, RFID_RX, RFID_TX);

  inventoryCmd[0] = 0xA0;  // head
  inventoryCmd[1] = 0x06;  // len
  inventoryCmd[2] = 0x01;  // reader address
  inventoryCmd[3] = 0x8B;  // cmd (undocumented inventory mode?)
  inventoryCmd[4] = 0x10;  // channels (TUNE THIS)
  inventoryCmd[5] = 0x00;  // tag a/b
  inventoryCmd[6] = 0xFF;  // repeat

  sendCommand(inventoryCmd, sizeof(inventoryCmd));

  // init display
  Wire.begin(SCREEN_SDA, SCREEN_SCL);
  if (!display.begin(SSD1309_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    println(F("SSD1309 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.display();
}
// //////////////////////////////////////////////////////////////////////////////

void sendCommand(uint8_t cmd[], size_t cmdSize) {
  cmd[cmdSize - 1] = checksum(cmd, cmdSize);
  RFID.write(cmd, cmdSize);
}

// return a checksum (to RFID reader's spec)
uint8_t checksum(const uint8_t* data, size_t length) {
  uint8_t sum = 0;

  for (size_t i = 0; i < length - 1; i++) {
    sum += data[i];
  }

  return (uint8_t)(~sum + 1);
}
// //////////////////////////////////////////////////////////////////////////////

void loop() {
  // to allow bidirectional communication with the demo software: (which contains config options)
  // we pipe ESP32 Serial input to the serial input of the reader
  // this allows us to send packets directly to and from the reader for debugging purposes,
  // sending any data to the esp's serial port triggers a special debugging 'quiet' mode (10s of quiet)
  // in this mode, we act only as a proxy between esp In and reader In
  // we take special care to only use our special print() and println() funcs, which respect quiet mode
  while (Serial.available()) {
    uint8_t b = Serial.read();
    RFID.write(b);

    // we need to go quiet on both comms TO the reader board
    RFIDCmdsQuietUntil = millis() + 10000;
    // and FROM our mcu board
    isESPSerialOutActive = false;
  }

  // when in quiet mode, we send reader data back out through the esp's serial out
  if (millis() < RFIDCmdsQuietUntil) {
    // pipe serial out to esp out
    while (RFID.available()) {
      uint8_t b = RFID.read();
      Serial.write(b);
    }
  }

  static unsigned long lastSend = 0;
  if (millis() - lastSend >= (1000 / 60)) {
    lastSend = millis();
    // prettyPrintKnownTags();
    // debugDisplayLoop(display);
    displayLoop(display);
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
// //////////////////////////////////////////////////////////////////////////////
enum TextAlign { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

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

// circular loading animation 1 character big
// frame: 0..7, cx/cy center, radius: distance of dots from center
void drawSpinner(DIYables_OLED_SSD1309& display, uint8_t frame, int16_t cx, int16_t cy, uint8_t radius) {
  const uint8_t N = 8;
  // clear small area around spinner
  int16_t pad = 3;
  display.fillRect(cx - radius - pad, cy - radius - pad, (radius + pad) * 2 + 1, (radius + pad) * 2 + 1, SSD1309_PIXEL_OFF);

  for (uint8_t i = 0; i < N; ++i) {
    float angle = 2.0f * PI * (float)i / (float)N;
    int16_t x = cx + (int16_t)round(cos(angle) * radius);
    int16_t y = cy + (int16_t)round(sin(angle) * radius);

    // highlight current frame with larger dot
    uint8_t dotR = (i == frame) ? 1 : 3;
    display.fillCircle(x, y, dotR, SSD1309_PIXEL_ON);
  }
}

// //////////////////////////////////////////////////////////////////////////////
constexpr uint8_t MaxEpcLength = 24;
constexpr uint8_t MaxTrackedTags = 16;
constexpr float RssiSmoothingAlpha = 0.2f;

struct Tag {
  uint8_t epc[MaxEpcLength];
  uint8_t epcLen;
  int rssi;            // signal strength
  float smoothedRssi;  // smoothed signal strength
  unsigned long lastSeen;
  int pings;                    // pings / repeat value = inventory cycles ran so far
  float pingsPerSec;            // average number of times the tag responded, per second of mcu runtime
  float pingsPerSecNormalized;  // normalize the above number, such that 1.0 is the most connected tag
                                //    and all others represent a proportion of that strength
};

struct SavedTag {
  uint8_t epc[MaxEpcLength];
  uint8_t epcLen;
  String name;
};

// all tags we've seen
extern Tag knownTags[MaxTrackedTags];
// the tags we care about
extern SavedTag savedTags[];
constexpr uint8_t maxSavedTags = 10;

SavedTag savedTags[maxSavedTags] = {
    {{0x79, 0xE7}, 2, "USB C"}, {{0x35, 0xE7}, 2, "CHRGR"}, {{0x6D, 0xE7}, 2, "LABEL"}, {{0x75, 0xE7}, 2, "TRACK"}, {{0x71, 0xE7}, 2, "TONER"}};

Tag knownTags[MaxTrackedTags] = {};

int16_t roundRssi(float value) { return (int16_t)(value < 0 ? value - 0.5f : value + 0.5f); }

bool epcEqual(const Tag& tag, const uint8_t* epc, uint8_t epcLen) {
  if (tag.epcLen != epcLen) return false;
  for (uint8_t i = 0; i < epcLen; ++i) {
    if (tag.epc[i] != epc[i]) return false;
  }
  return true;
}

bool epcTailEqual(const uint8_t* epcA, uint8_t lenA, const uint8_t* epcB, uint8_t lenB, uint8_t tailLen) {
  if (lenA < tailLen || lenB < tailLen) return false;
  return memcmp(epcA + lenA - tailLen, epcB + lenB - tailLen, tailLen) == 0;
}

uint16_t getTagTailId(const Tag& tag) {
  if (tag.epcLen < 2) return 0;
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

void dumpHex() {
  print("RAW: ");
  for (int i = 0; i < recvIndex; i++) {
    if (recvBuffer[i] < 0x10) print("0");
    print(recvBuffer[i], HEX);
    print(" ");
  }
  println();
}

void reportTag(const uint8_t* epc, uint8_t epcLen, int8_t rssi) {
  unsigned long now = millis();
  uint8_t normalizedEpc[MaxEpcLength];
  uint8_t normalizedLen = normalizeEpc(epc, epcLen, normalizedEpc);

  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    if (epcEqual(knownTags[i], normalizedEpc, normalizedLen)) {
      knownTags[i].smoothedRssi = knownTags[i].smoothedRssi * (1.0f - RssiSmoothingAlpha) + rssi * RssiSmoothingAlpha;
      knownTags[i].rssi = rssi;
      knownTags[i].lastSeen = now;
      knownTags[i].pings++;
      // knownTags[i].pingsPerSec = now / 1000 > 0 ? ((float)knownTags[i].pings / (now / 1000)) : 0;
      return;
    }
  }

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

void parseSuccess() {
  // there is a ramp up (inventory start up overhead on the reader) when calling this.
  // there is probably a better way to do this
  sendCommand(inventoryCmd, sizeof(inventoryCmd));
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

void prettyPrintKnownTags() {
  println("KNOWN TAGS:");
  for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
    const Tag& tag = knownTags[i];
    if (tag.epcLen == 0 || millis() - tag.lastSeen > 10000) {
      continue;
    }

    uint16_t tailId = getTagTailId(tag);
    print("  [");
    printTwoHexBytes(tailId);
    print("] ");
    print("RSSI=");
    print(tag.rssi);
    print("  smoothed=");
    print(tag.smoothedRssi, 1);
    print("  pings=");
    print(tag.pings);
    print("  ageMs=");
    print(millis() - tag.lastSeen);
    println();
  }
}

// //////////////////////////////////////////////////////////////////////////////
void displayLoop(DIYables_OLED_SSD1309& display) {
  display.clearDisplay();
  // display.setTextSize(1);
  // display.setTextColor(SSD1309_PIXEL_ON);

  uint8_t spinnerFrame = (millis() / 120) % 8;
  drawSpinner(display, spinnerFrame, 64, 32, 16);

  display.display();
}

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

    for (uint8_t savedIndex = 0; savedIndex < 10; savedIndex++) {
      if (savedTags[savedIndex].epcLen == 0) continue;

      if (epcTailEqual(knownTags[i].epc, knownTags[i].epcLen, savedTags[savedIndex].epc, savedTags[savedIndex].epcLen, 2)) {
        id = savedTags[savedIndex].name;
        String tag = id + ((millis() - knownTags[i].lastSeen > 1000) ? String(" X ") : (String(" ") + String(knownTags[i].pingsPerSecNormalized, 2)));
        rssiSum += knownTags[i].smoothedRssi;
        rssiTotalCount++;
        drawJustifiedText(display, tag, y, align);
        // drawJustifiedText(display, String(knownTags[i].pingsPerSecNormalized, 2), y, ALIGN_RIGHT);
        y += 7;
        if (y >= 63) {
          y = 0;
          align = ALIGN_RIGHT;
        }
      }
    }
  }

  // align = ALIGN_RIGHT;
  // y = 0;
  // for (uint8_t i = 0; i < MaxTrackedTags; ++i) {
  //   if (knownTags[i].pings == 0) continue;
  //   if (knownTags[i].epcLen == 0) continue;
  //   uint16_t tailId = getTagTailId(knownTags[i]);
  //   String id = "";
  //   if (tailId < 0x1000) id += "0";
  //   if (tailId < 0x0100) id += "0";
  //   if (tailId < 0x0010) id += "0";
  //   id += String(tailId, HEX);

  //   // for (uint8_t savedIndex = 0; savedIndex < 10; savedIndex++) {
  //   // if (savedTags[savedIndex].epcLen == 0) continue;

  //   // if (epcTailEqual(knownTags[i].epc, knownTags[i].epcLen, savedTags[savedIndex].epc, savedTags[savedIndex].epcLen, 2)) {
  //   String tag = id + ((millis() - knownTags[i].lastSeen > 1000) ? String(" X ") : (String(" ") + String(knownTags[i].pingsPerSecNormalized, 2)));

  //   rssiSum += knownTags[i].smoothedRssi;
  //   rssiTotalCount++;
  //   drawJustifiedText(display, tag, y, align);
  //   // drawJustifiedText(display, String(knownTags[i].pingsPerSecNormalized, 2), y, ALIGN_RIGHT);
  //   y += 7;
  //   if (y >= 63) {
  //     y = 0;
  //     align = ALIGN_RIGHT;
  //   }
  //   // }
  //   // }
  // }

  // drawJustifiedText(display, String(rssiSum / rssiTotalCount), 57, ALIGN_RIGHT);
  // drawJustifiedText(display, String(inventoryChannel), 57, ALIGN_CENTER);

  uint8_t spinnerFrame = (millis() / 120) % 8;
  drawSpinner(display, spinnerFrame, 120, 56, 4);  // adjust cx,cy and radius to taste

  display.display();
}