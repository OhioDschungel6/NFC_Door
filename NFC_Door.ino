
#include <Arduino.h>
#include <ESPmDNS.h>
#include <MFRC522Extended.h>
#include <SPI.h>

#include "Android.h"
#include "Desfire.h"
#include "NetworkClient.h"
#include "NetworkManager.h"
#include "Utils.h"

// Libraries Webserver
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "WiFi.h"

#define RST_PIN 21
#define SS_PIN 5


MFRC522Extended mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;

boolean isPresent = false;
IPAddress serverIp;
unsigned int serverPort;


boolean Reader = true;
boolean Writer = false;

unsigned char presharedKey[16];

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  if (!SPIFFS.begin()) {
    Serial.println("Mounting spiffs failed");
    return;
  }
  if (!getConfiguration()) {
    Serial.println("Config failed");
    //TODO exception schmeißen
  }
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println(F("Setup ready"));
}

boolean getConfiguration() {
  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    Serial.println("Failed to read file");
    return false;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to read file, using default configuration"));
    Serial.println(error.c_str());
    file.close();
    return false;
  }

  if (doc.containsKey("Wifi") && doc["Wifi"].containsKey("ssid") && doc["Wifi"].containsKey("pw")
      && !String().equals(doc["Wifi"]["ssid"].as<String>()) && !String().equals(doc["Wifi"]["pw"].as<String>())) {
    Serial.println("Using wifi settings from config file");
    String ssid = doc["Wifi"]["ssid"].as<String>();
    String pw = doc["Wifi"]["pw"].as<String>();
    WiFi.mode(WIFI_STA);
    wl_status_t wifiStatus = WiFi.begin(ssid.c_str(), pw.c_str());
    if(WL_CONNECTED != wifiStatus){
      Serial.println("Error connecting to wifi.");
      Serial.println(wifiStatus);
      Serial.println(ssid);
      Serial.println(pw);
      return false;      
    }
  } else {
    Serial.println("Start networkmanager");
    NetworkManager networkManager;
    networkManager.Setup();
  }

  if (doc.containsKey("Doorserver") && doc["Doorserver"].containsKey("ip") && doc["Doorserver"].containsKey("port")
      && !String().equals(doc["Doorserver"]["ip"].as<String>()) && !String().equals(doc["Doorserver"]["port"].as<String>())) {
    //use config data from user
    Serial.println("Using config file for server config");
    serverIp = IPAddress();
    serverIp.fromString(doc["Doorserver"]["ip"].as<String>());
    serverPort = doc["Doorserver"]["port"].as<unsigned int>();
    Serial.println("Server address:");
    Serial.print(serverIp);
    Serial.print(":");
    Serial.println(serverPort);
  } else {
    //Automatically get ip and port from mdns
    Serial.println("Get server config with mdns");
    if (!getServerConfigFromMDns()) {
      file.close();
      return false;
    }
  }
 

  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(100);
  }
  if(!doc.containsKey("secretkey")){
    Serial.println("No key found in config file");
    return false;
  }
  String key = doc["secretkey"].as<String>();
  if(key.length() != 32){
    Serial.println("Keylength has to be 16 byte");
    return false;
  }
  hex2bin(key.c_str(),presharedKey);

  file.close();

  if (Writer) {
    return startWebServer();
  }
  return true;
}

boolean getServerConfigFromMDns() {
  if (mdns_init() != ESP_OK) {
    Serial.println("mDNS failed to start");
    return false;
  }
  const char *proto = "tcp";
  const char *service = "homekeypro";
  Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
  int n = MDNS.queryService(service, proto);
  if (n == 0) {
    Serial.println("no services found");
    return false;
  }
  serverIp = MDNS.IP(0);
  serverPort = MDNS.port(0);
  Serial.println("Server address:");
  Serial.print(serverIp);
  Serial.print(":");
  Serial.println(serverPort);
  return true;
}

boolean startWebServer() {

  auto server = new AsyncWebServer(80);
  // Route for root / web page
  server->on("/api/chips", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    NetworkClient client(serverIp, serverPort);
    byte cmd = 0x6D;
    client.Send(&cmd, 1);
    int length;
    client.Recieve((byte *)&length, 4);
    byte *devicesJson = new byte[length];
    client.Recieve(devicesJson, length);
    response->write(devicesJson, length);
    request->send(response);
    delete[] devicesJson;
  });

  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Delivering default page");
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server->on("/new", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server->on("/new/search", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server->on("/new/success", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server->on("/new/failure", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server->on("/overview", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server->serveStatic("/fs/", SPIFFS, "/fs/");

  server->onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (request->url() == "/api/chip") {
      handleChipApi(request, data);
    } else {
      request->send(404);
      return;
    }
  });

  Serial.println("Starting server NOW...");
  // Start server
  server->begin();
  if (mdns_init() != ESP_OK) {
    Serial.println("mDNS failed to start");
    return false;
  }
  if (!MDNS.begin("home-key-pro")) {
    Serial.println("Error starting mDNS");
    return false;
  }
  MDNS.addService("http", "tcp", 80);
  return true;
}

void handleChipApi(AsyncWebServerRequest *request, uint8_t *data) {
  if (request->method() != HTTP_POST && request->method() != HTTP_DELETE) {
    request->send(405);
    return;
  }
  DynamicJsonDocument jsonBuffer(100);
  DeserializationError error = deserializeJson(jsonBuffer, (const char *)data);
  if (request->method() == HTTP_POST) {
    if (error || !jsonBuffer.containsKey("name")) {
      request->send(201);
      return;
    }
    RegisterResult result = registerDevice(jsonBuffer["name"].as<String>());
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument jsonBuffer(100);
    jsonBuffer["result"] = registerResultToString(result);
    serializeJson(jsonBuffer, *response);
    request->send(response);
  } else if (request->method() == HTTP_DELETE) {
    if (error || !jsonBuffer.containsKey("uid")) {
      request->send(201);
      return;
    }
    String hexStringUid = jsonBuffer["uid"].as<String>();
    byte uid[16] = {0};
    hex2bin(hexStringUid.c_str(), uid);
    deleteDevice(uid);
    request->send(200);
  }
}

void deleteDevice(byte uid[]) {
  NetworkClient client(serverIp, serverPort);
  byte cmd = 0xDD;
  client.Send(&cmd, 1);
  client.SendWithHMAC(uid, 16, presharedKey);
}

RegisterResult registerDevice(String name) {
  if (name.length() == 0) {
    return RegisterResult_Failed;
  }
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return RegisterResult_NoCard;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return RegisterResult_NoCard;
  }
  if (mfrc522.uid.sak != 0x20) {
    Serial.println("Non compatible card found");
    mfrc522.PICC_HaltA();
    return RegisterResult_CardNotCompatible;
  }
  if (mfrc522.uid.size == 4) {
    Serial.println("Android detected");
    return registerAndroidDevice(name);
  } else if (mfrc522.uid.size == 7) {
    Serial.println("Desfire detected");
    return registerDesfireDevice(name);
  }
  return RegisterResult_CardNotCompatible;
}

RegisterResult registerAndroidDevice(String name) {
  Android android = Android(&mfrc522, serverIp, serverPort);
  // TODO Change aid?
  byte aid[] = {0xF0, 0x39, 0x41, 0x45, 0x32, 0x81, 0x00};
  if (!android.SelectApplication(aid)) {
    Serial.println("Android Error");
    return RegisterResult_AppNotInstalled;
  }
  if (android.IsAndroidKnown()) {
    return RegisterResult_AlreadyRegistered;
  }
  if (!android.GetKey(name, presharedKey)) {
    return RegisterResult_NfcError;
  }
  return RegisterResult_Ok;
}

RegisterResult registerDesfireDevice(String name) {
  Desfire desfire = Desfire(&mfrc522, serverIp, serverPort);
  KeySettings keySettings;
  if (!desfire.GetKeySettings(&keySettings)) {
    return RegisterResult_NfcError;
  }

  KeyType masterKeyType = keySettings.keyType;
  if (!desfire.AuthenticateNetwork(masterKeyType, 0)) {
    // Could not authentificate against masterkey
    // check if applications can be created
    Serial.println("Foreign card");
    // Check for permissions "list applications without MK" and "create application without MK"
    if ((keySettings.secSettings & 0x06) != 0x06) {
      Serial.println("Not allowed to create applications without masterkey");
      return RegisterResult_CardNotCompatible;
    }
  } else if (!desfire.IsKeyKnown()) {
    // We could authentificate, but we dont know the key ==> key is default key
    // Change masterkeytype
    Serial.println("Change key, because key was default");
    if (!desfire.ChangeKeyNetwork(KEYTYPE_AES, name, presharedKey)) {
      // should not happen
      return RegisterResult_NfcError;
    }
  }
  uint32_t appId = desfire.GetAppIdFromNetwork();
  if (appId == 0) {
    // Card is unknown to server
    uint32_t appIds[32];
    int anz_ids = desfire.GetAppIds(appIds, 32);
    uint32_t nextAppID = getNextFreeAppId(appIds, anz_ids);
    if (!desfire.CreateApplication(nextAppID, 1, KEYTYPE_AES)) {
      Serial.println("Create app failed");
      return RegisterResult_NfcError;
    }
    if (!desfire.SelectApplication(nextAppID)) {
      Serial.println("Select app failed");
      return RegisterResult_NfcError;
    }
    if (!desfire.AuthenticateNetwork(KEYTYPE_AES, 0)) {
      Serial.println("Auth failed");
      return RegisterResult_NfcError;
    }
    // Register card on server
    if (!desfire.ChangeKeyNetwork(KEYTYPE_AES, name, presharedKey)) {
      return RegisterResult_NfcError;
    }
    return RegisterResult_Ok;
  } else {
    // Card already known to server
    if (!desfire.SelectApplication(appId)) {
      return RegisterResult_NfcError;
    }
    if (!desfire.AuthenticateNetwork(KEYTYPE_AES, 0)) {
      return RegisterResult_NfcError;
    }
    return RegisterResult_AlreadyRegistered;
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
      Android android = Android(&mfrc522, serverIp, serverPort);
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
      Desfire desfire = Desfire(&mfrc522, serverIp, serverPort);
      uint32_t appId = desfire.GetAppIdFromNetwork();
      if (appId == 0) {
        // Card unknown
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
