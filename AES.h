#pragma once
#include <Arduino.h>
#include <aes/esp_aes.h>

class AES {
   public:
    AES();

    int setKey(const byte *key);
    void setIV(const byte iv[16]);

    int encryptCBC(size_t length, const byte *input, byte *output);
    int decryptCBC(size_t length, const byte *input, byte *output);

   private:
    esp_aes_context context;
    byte iv[16];
};
