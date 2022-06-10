#include "AES.h"

#include <Arduino.h>
#include <aes/esp_aes.h>

AES::AES() {
    esp_aes_init(&this->context);
}
int AES::setKey(const byte *key) {
    return esp_aes_setkey(&this->context, key, 128);
}
void AES::setIV(const byte iv[16]) {
    memcpy(this->iv, iv, 16);
}
int AES::encryptCBC(size_t length, const byte *input, byte *output) {
    return esp_aes_crypt_cbc(&this->context, ESP_AES_ENCRYPT, length, iv, input, output);
}
int AES::decryptCBC(size_t length, const byte *input, byte *output) {
    return esp_aes_crypt_cbc(&this->context, ESP_AES_DECRYPT, length, iv, input, output);
}
