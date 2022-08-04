#include "NetworkClient.h"

#include <DNSServer.h>

// String host = "192.168.178.22";
// Regensburg
//IPAddress host(192, 168, 178, 22);
// Bruck
//IPAddress host(192,168,179,85);
unsigned int port = 80;

void dumpInfo(byte *ar, int len);

NetworkClient::NetworkClient(String ip) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wifi not available");
        return;
    }
    if (!client.connect(ip.c_str(), port)) {
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
