#include "Utils.h"

void dumpInfo(byte* ar, int len) {
    for (int i = 0; i < len; i++) {
        if (ar[i] < 0x10)
            Serial.print(F("0"));
        Serial.print(ar[i], HEX);
        Serial.print(F(" "));
    }
    Serial.println("");
}

uint32_t parseAppId(const byte* buffer) {
    return buffer[0] | buffer[1] << 8 | buffer[2] << 16;
}
