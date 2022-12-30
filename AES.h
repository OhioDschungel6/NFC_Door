#pragma once
#include <Arduino.h>
#include <aes/esp_aes.h>

class AES {
   public:
    AES();

    int setKey(const unsigned char *key, unsigned int keybits);
    void setIV(byte *iv);

    int encryptCBC(size_t length, const unsigned char *input, unsigned char *output);
    int decryptCBC(size_t length, const unsigned char *input, unsigned char *output);

   private:
    esp_aes_context context;
    byte iv[16];
};
