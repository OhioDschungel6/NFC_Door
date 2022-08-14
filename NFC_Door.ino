
#include <Arduino.h>
#include <MFRC522Extended.h>
#include <SPI.h>

#include "Android.h"
#include "Desfire.h"
#include "NetworkClient.h"
#include "NetworkManager.h"
#include "Utils.h"

#include <ESPmDNS.h>

//Libraries Webserver
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "WiFi.h"

#define RST_PIN 21
#define SS_PIN 5

NetworkManager networkManager;
MFRC522Extended mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;

boolean isPresent = false;
IPAddress serverIp;
unsigned int serverPort;
AsyncWebServer server(80);

boolean Reader = false;
boolean Writer = true;

const unsigned char *presharedKey = (const unsigned char *)"secretKey1234567";

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  networkManager.Setup();
  SPI.begin();

  findServerIP();
  if (Writer) {
    startWebServer();
  }

  mfrc522.PCD_Init();
  Serial.println(F("Setup ready"));
}

void findServerIP() {
  if (mdns_init() != ESP_OK) {
    Serial.println("mDNS failed to start");
    return;
  }
  const char * proto = "tcp";
  const char * service = "homekeypro";
  Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
  int n = MDNS.queryService(service, proto);
  if (n == 0) {
    Serial.println("no services found");
  }
  serverIp = MDNS.IP(0);
  serverPort = MDNS.port(0);
  Serial.println("Server address:");
  Serial.print(serverIp);
  Serial.print(":");
  Serial.println(serverPort);

}

void startWebServer() {
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Delivering default page");
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/new", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/chipFound", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/overview", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/api/chips", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    NetworkClient client(serverIp,serverPort);
    byte cmd = 0x6D;
    client.Send(&cmd, 1);
    int length;
    client.Recieve((byte*)&length, 4);
    byte* devicesJson = new byte[length];
    client.Recieve(devicesJson, length);
    response->write(devicesJson, length);
    request->send(response);
    delete[] devicesJson;
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/new", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/chipSearch", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/chipFound", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/overview", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.serveStatic("/static/", SPIFFS, "/static/");
  server.serveStatic("/fs/", SPIFFS, "/fs/");

  server.onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (request->url() == "/api/chip") {
      if (request->method() == HTTP_POST) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject((const char *)data);
        if (root.success()) {
          if (root.containsKey("name")) {
            if (registerDevice(root["name"].asString())) {
              request->send(200);
              return;
            }
          }
        }
        request->send(201);
        return;
      } else if (request->method() == HTTP_DELETE) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject((const char *)data);
        if (root.success()) {
          if (root.containsKey("uid")) {
            String hexStringUid = root["uid"].asString();
            byte uid[16];
            hex2bin(hexStringUid.c_str(), uid);
            deleteDevice(uid);
            request->send(200);
            return;
          }
        }
        request->send(201);
        return;
      }
    }
    request->send(404);
  });

  Serial.println("Starting server NOW...");
  // Start server
  server.begin();
  if(mdns_init()!= ESP_OK){
    Serial.println("mDNS failed to start");
    return;
  }
  if(!MDNS.begin("Home-key-pro")) {
     Serial.println("Error starting mDNS");
     return;
  }
  MDNS.addService("http", "tcp", 80);
}

void deleteDevice(byte uid[]) {
  NetworkClient client(serverIp,serverPort);
  byte cmd = 0xDD;
  client.Send(&cmd, 1);
  byte ekNonce [16];
  byte nonce[17];
  client.Recieve(ekNonce, 16);
  AES32 sharedKeyDecryptor;
  sharedKeyDecryptor.setKey(presharedKey, 128);
  byte iv [16] = {0};
  sharedKeyDecryptor.setIV(iv);
  sharedKeyDecryptor.decryptCBC(16, ekNonce, nonce + 1);
  dumpInfo(nonce + 1, 16);
  nonce[0] = nonce[16];

  AES32 sharedKeyEncryptor;
  sharedKeyEncryptor.setKey(presharedKey, 128);
  sharedKeyEncryptor.setIV(iv);

  byte msg[32];
  memcpy(msg, nonce, 16);
  memcpy(msg + 16, uid, 16);

  byte msgEnc[32];
  sharedKeyEncryptor.encryptCBC(32, msg, msgEnc);
  client.Send(msgEnc, 32);
}

boolean registerDevice(String name) {
  if (name.length() == 0) {
    return false;
  }
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return false;
  }
  if (mfrc522.uid.sak != 0x20) {
    Serial.println("Non compatible card found");
    mfrc522.PICC_HaltA();
    return false;
  }
  if (mfrc522.uid.size == 4) {
    Serial.println("Android detected");
    return registerAndroidDevice(name);
  } else if (mfrc522.uid.size == 7) {
    Serial.println("Desfire detected");
    return registerDesfireDevice(name);
  }
  return false;
}

boolean registerAndroidDevice(String name) {
  Android android = Android(&mfrc522, serverIp,serverPort);
  byte aid[] = {0xF0, 0x39, 0x41, 0x45, 0x32, 0x81, 0x00};
  if (!android.SelectApplication(aid)) {
    Serial.println("Android Error");
    return false;
  }
  return android.GetKey(name, presharedKey);
}

boolean registerDesfireDevice(String name) {
  Desfire desfire = Desfire(&mfrc522, serverIp,serverPort);
  uint32_t appId = 0;
  KeySettings keySettings;
  if (!desfire.GetKeySettings(&keySettings)) {
    return false;
  }
  appId = desfire.GetAppIdFromNetwork();
  KeyType masterKeyType = keySettings.keyType;
  if (!desfire.AuthenticateNetwork(masterKeyType, 0)) {
    //Could not authentificate against masterkey
    //check if applications can be created
    Serial.println("Foreign card");
    if((keySettings.secSettings & 0x06) !=0x06){
      Serial.println("Not allowed to create applications without masterkey");
      return false;
    }
  } else if (!desfire.IsKeyKnown()) {
    //We could authentificate, but we dont know the key ==> key is default key
    //Change masterkeytype
    Serial.println("Change key, because key was default");
    if (!desfire.ChangeKeyNetwork(KEYTYPE_AES, name, presharedKey)) {
      // should not happen
      return false;
    }
  }
  
  if (appId == 0) {
    //Card is unknown to server
    uint32_t appIds[32];
    int anz_ids = desfire.GetAppIds(appIds, 32);
    uint32_t nextAppID = getNextFreeAppId(appIds, anz_ids);
    if (!desfire.CreateApplication(nextAppID, 1, KEYTYPE_AES)) {
      Serial.println("Create app failed");
      return false;
    }
    if (!desfire.SelectApplication(nextAppID)) {
      Serial.println("Select app failed");
      return false;
    }
    if (!desfire.AuthenticateNetwork(KEYTYPE_AES, 0)) {
      Serial.println("Auth failed");
      return false;
    }
    //Register card on server
    return desfire.ChangeKeyNetwork(KEYTYPE_AES, name, presharedKey);
  } else {
    //Card already known to server
    if (!desfire.SelectApplication(appId)) {
      return false;
    }
    return desfire.AuthenticateNetwork(KEYTYPE_AES, 0);
  }
}

void loop() {
  if (!Reader) {
    return;
  }
  if (!isPresent) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
    }
    Serial.println("New picc present");
    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      return;
    }
    if (mfrc522.uid.sak != 0x20) {
      return;
    }
    isPresent = true;
    if (mfrc522.uid.size == 4) {
      Serial.println("Android detected");
      Android android = Android(&mfrc522, serverIp,serverPort);
      Buffer<7> buffer;
      byte aid[] = {0xF0, 0x39, 0x41, 0x45, 0x32, 0x81, 0x00};
      if (!android.SelectApplication(aid)) {
        Serial.println("Select failed");
        return;
      }
      if (!android.Verify()) {
        Serial.println("Verify failed");
        return;
      }
    } else if (mfrc522.uid.size == 7) {
      Serial.println("Desfire detected");
      Desfire desfire = Desfire(&mfrc522, serverIp,serverPort);
      uint32_t appId = desfire.GetAppIdFromNetwork();
      if (appId == 0) {
        //Card unknown
        return;
      }
      if (!desfire.SelectApplication(appId)) {
        return;
      }
      if (!desfire.AuthenticateNetwork(KEYTYPE_AES, 0)) {
        return;
      }
    } else {
      Serial.println(F("Other Card found, not compatible!"));
      mfrc522.PICC_HaltA();
      isPresent = false;
      return;
    }
  }
  if (isPresent) {  // test read - it it fails, the PICC is most likely gone
    byte buffer[20] = {0};
    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(0, buffer, &byteCount);
    if (status != MFRC522::STATUS_OK) {
      isPresent = false;
      mfrc522.PCD_StopCrypto1();
      Serial.println("Card gone...");
      return;
    }
  }
}

uint32_t getNextFreeAppId(uint32_t appIds[], int length) {
  if (length == 0) {
    return 1;
  }
  qsort(appIds, length, sizeof(uint32_t), sort_asc);
  if (appIds[0] != 1) {
    return 1;
  }
  for (int i = 0; i < length - 1; i++) {
    if (appIds[i] < appIds[i + 1] - 1) {
      return appIds[i] + 1;
    }
  }
  return appIds[length - 1] + 1;
}

int sort_asc(const void *cmp1, const void *cmp2) {
  // Need to cast the void * to int *
  int a = *((uint32_t *)cmp1);
  int b = *((uint32_t *)cmp2);
  // The comparison
  return a < b ? -1 : (a > b ? 1 : 0);
  // A simpler, probably faster way:
  //return b - a;
}
