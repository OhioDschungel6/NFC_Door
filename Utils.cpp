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


int char2int(char input)
{
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  return 0;
}

void hex2bin(const char* src, byte* target)
{
  while(*src && src[1])
  {
    *(target++) = char2int(*src)*16 + char2int(src[1]);
    src += 2;
  }
}
