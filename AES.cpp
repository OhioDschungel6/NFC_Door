#include <AES.h>

AES::AES() {
    esp_aes_init(&this->context);
}

int AES::setKey(const unsigned char *key) {
    return esp_aes_setkey(&this->context, key, 128);
}

int AES::encryptCBC(size_t length, const unsigned char *input, unsigned char *output) {
    return esp_aes_crypt_cbc(&this->context, ESP_AES_ENCRYPT, length, iv, input, output);
}

int AES::decryptCBC(size_t length, const unsigned char *input, unsigned char *output) {
    return esp_aes_crypt_cbc(&this->context, ESP_AES_DECRYPT, length, iv, input, output);
}

void AES::setIV(byte *iv_p) {
    memcpy(iv, iv_p, 16);
}