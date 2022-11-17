#include "NetworkClient.h"

#include <DNSServer.h>
#include "Utils.h"
#include "mbedtls/md.h"


NetworkClient::NetworkClient(IPAddress ip,unsigned int port) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wifi not available");
        return;
    }
    if (!client.connect(ip, port)) {
        // Error;
    }
}

void NetworkClient::Send(byte message[], int size) {
    // Serial.println("Sending message:");
    // dumpInfo(message,size);
    client.write((char *)message, size);
    client.flush();
}

void NetworkClient::SendWithHMAC(byte message[], int size, const unsigned char * presharedKey) {
    // Serial.println("Sending message:");
    dumpInfo(message,size);
    byte nonce [32];
    Recieve(nonce,32);
    dumpInfo(nonce,32);
    Buffer<4> sizeBuffer;
    sizeBuffer.append32(size);
    client.write(sizeBuffer.buffer,sizeBuffer.size);
    dumpInfo(sizeBuffer.buffer,sizeBuffer.size);
    client.write((char *)message, size);
    //HMAC
    byte hmacResult[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, presharedKey, 16);
    mbedtls_md_hmac_update(&ctx, (const unsigned char *) message, size);
    mbedtls_md_hmac_update(&ctx, (const unsigned char *) nonce, 32);
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
    dumpInfo(hmacResult,32);
    client.write(hmacResult,32);
    client.flush();
}

int NetworkClient::Recieve(byte buffer[], int n) {
    while (client.available() < n) {
        delay(1);
    }
    // Serial.println("Message received:");
    client.read(buffer, n);
    // dumpInfo(buffer,n);
    return 0;
}
