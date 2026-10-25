#include "tag_config.h"

// tags are matched by the least significant bytes of the EPC, so the epcLen field is used to determine how many bytes to compare.

SavedTag savedTags[] = {{{0x79, 0xE7}, 2, "USBC"},
                        {{0x35, 0xE7}, 2, "CHRG"},
                        {{0xD7, 0xB6, 0x69, 0xE7}, 4, "BADGE"},
                        {{0x75, 0xE7}, 2, "TRACR"},
                        {{0x71, 0xE7}, 2, "TONR"},
                        {{0xD7, 0xB6, 0x65, 0xE7}, 4, "LABEL"},
                        {{0xD7, 0xB6, 0x61, 0xE7}, 4, "GREY"},
                        {{0xD7, 0xB6, 0x3D, 0xE7}, 4, "BLCK"}};
//

const uint8_t maxSavedTags = sizeof(savedTags) / sizeof(savedTags[0]);