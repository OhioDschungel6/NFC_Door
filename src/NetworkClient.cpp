#include <DNSServer.h>
#include "NetworkClient.h"

//String host = "192.168.178.22";
IPAddress host(192,168,178,22);
unsigned int port = 80;

NetworkClient::NetworkClient()
{
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi not available");
    return;
  }
  if(!client.connect(host, port)){
    //Error;
  }
}

void NetworkClient::Send(byte message[],int size)
{
  Serial.println("Sending message");
  client.write((char*)message,size);
  client.flush();
}

int NetworkClient::Recieve(byte buffer[],int n)
{
  while(client.available()<n){
    delay(1);
  }
  Serial.println("Message received");
  client.read(buffer,n);
  return 0;
}
