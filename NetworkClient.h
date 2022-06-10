#pragma once
#include <Arduino.h>
#include <WiFi.h>

class NetworkClient {
   public:
    NetworkClient();
    void Send(byte[], int);
    int Recieve(byte[], int);

   private:
    WiFiClient client;  // Use WiFiClient class to create TCP connections
};
