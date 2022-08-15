//=== WIFI MANAGER ===
#include <DNSServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
char wifiManagerAPName[] = "NFCReader";
char wifiManagerAPPassword[] = "NFCReader";

//== SAVING CONFIG ==
#include <ArduinoJson.h>

#include "SPIFFS.h"
bool shouldSaveConfig = false;  // flag for saving data

// callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

#include <WiFi.h>

#include "NetworkManager.h"

#define DEBUG 0

void configModeCallback(WiFiManager* myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());

    // You could indicate on your screen or by an LED you are in config mode here
}

bool loadConfig() {
    File configFile = SPIFFS.open("/config1.json", "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return false;
    }

    size_t size = configFile.size();
    if (size > 1024) {
        Serial.println("Config file size is too large");
        return false;
    }

    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    configFile.readBytes(buf.get(), size);

    StaticJsonDocument<200> jsonBuffer;
    DeserializationError error = deserializeJson(jsonBuffer, buf.get());
    if (error) {
        Serial.println("Failed to parse config file");
        return false;
    }
    return true;
}

bool saveConfig() {
    StaticJsonDocument<200> jsonBuffer;
    deserializeJson(jsonBuffer,"{}");
    File configFile = SPIFFS.open("/config1.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    serializeJson(jsonBuffer, configFile);
    return true;
}

NetworkManager::NetworkManager() {
}

void NetworkManager::Setup() {
    //-- Config --
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount FS");
        return;
    }
    loadConfig();

    //-- WiFiManager --
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //wifiManager.setHttpPort(8080);

     // fetches ssid and pass from eeprom and tries to connect
     // if it does not connect it starts an access point with the specified name wifiManagerAPName
     // and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect(wifiManagerAPName, wifiManagerAPPassword);
    

    //-- Status --
    Serial.println("WiFi connected");

    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    if (shouldSaveConfig) {
        saveConfig();
    }
    wifiManager.stopWebPortal();
    delay(3000);
}
