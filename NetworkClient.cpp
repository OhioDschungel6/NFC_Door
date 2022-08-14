#include "NetworkClient.h"

#include <DNSServer.h>

void dumpInfo(byte *ar, int len);

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

int NetworkClient::Recieve(byte buffer[], int n) {
    while (client.available() < n) {
        delay(1);
    }
    // Serial.println("Message received:");
    client.read(buffer, n);
    // dumpInfo(buffer,n);
    return 0;
}
