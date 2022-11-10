#pragma once
#include <Arduino.h>
#include <WiFi.h>

class NetworkClient {
   public:
    NetworkClient(IPAddress ip, unsigned int port);
    void Send(byte[], int);
    void SendWithHMAC(byte message[], int size, const unsigned char * presharedKey);
    int Recieve(byte[], int);

   private:
    WiFiClient client;
};
