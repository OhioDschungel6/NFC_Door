#include "Utils.h"

const char* registerResultToString(RegisterResult result) {
    switch (result) {
        case RegisterResult_Ok:
            return "Ok";
        case RegisterResult_AlreadyRegistered:
            return "AlreadyRegistered";
        case RegisterResult_Failed:
            return "Failed";
        case RegisterResult_NoCard:
            return "NoCard";
        case RegisterResult_CardNotCompatible:
            return "CardNotCompatible";
        case RegisterResult_AppNotInstalled:
            return "AppNotInstalled";
        case RegisterResult_NfcError:
            return "NfcError";
    }
    return "";
}

void dumpInfo(byte* ar, int len) {
    for (int i = 0; i < len; i++) {
        if (ar[i] < 0x10)
            Serial.print(F("0"));
        Serial.print(ar[i], HEX);
        Serial.print(F(" "));
    }
    Serial.println("");
}

int char2int(char input) {
    if (input >= '0' && input <= '9')
        return input - '0';
    if (input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if (input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    return 0;
}

void hex2bin(const char* src, byte* target) {
    while (*src && src[1]) {
        *(target++) = char2int(*src) * 16 + char2int(src[1]);
        src += 2;
    }
}

uint32_t parseAppId(const byte* buffer) {
    return buffer[0] | buffer[1] << 8 | buffer[2] << 16;
}

int sort_asc(const void* cmp1, const void* cmp2) {
    int a = *((uint32_t*)cmp1);
    int b = *((uint32_t*)cmp2);
    return a < b ? -1 : (a > b ? 1 : 0);
}

uint32_t getNextFreeAppId(uint32_t appIds[], int length) {
    if (length == 0) {
        return 1;
    }
    qsort(appIds, length, sizeof(uint32_t), sort_asc);
    if (appIds[0] != 1) {
        return 1;
    }
    for (int i = 0; i < length - 1; i++) {
        if (appIds[i] < appIds[i + 1] - 1) {
            return appIds[i] + 1;
        }
    }
    return appIds[length - 1] + 1;
}
