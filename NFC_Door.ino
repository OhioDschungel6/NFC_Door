
#include <Arduino.h>
#include <MFRC522Extended.h>
#include <SPI.h>

#include "Android.h"
#include "Desfire.h"
#include "NetworkClient.h"
#include "NetworkManager.h"
#include "Utils.h"

#define RST_PIN 21
#define SS_PIN 5

NetworkManager networkManager;
MFRC522Extended mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;

boolean isPresent = false;
String ip;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  networkManager.Setup();
  SPI.begin();

  findServerIP();
  mfrc522.PCD_Init();
  Serial.println(F("Setup ready"));
}

void findServerIP() {
  byte buffer[255] = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 1\r\nST: server";
  WiFiUDP udp;
  const char * udpAddress = "239.255.255.250";
  const int udpPort = 1900;
  udp.beginPacket(udpAddress, udpPort);
  
  udp.write(buffer, 88);
  udp.endPacket();
  delay(1000);
  int size = udp.parsePacket();
  udp.read(buffer,size);
  String answer = String( (char *)buffer); 
  int startIndex = answer.indexOf("LOCATION:");
  if(startIndex== -1){
    return;
  }
  String subStringAnswer = answer.substring(startIndex);
  int endIndex = subStringAnswer.indexOf('\n');
  if(endIndex != -1){
    subStringAnswer = subStringAnswer.substring(0,endIndex);
  }
  ip = subStringAnswer.substring(subStringAnswer.indexOf(':')+1);
  ip.trim();
  Serial.println("Server ip is:");
  Serial.println(ip);
}


void loop() {
  if (!isPresent) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
    }
    Serial.println("New picc present");
    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      return;
    } else {
      isPresent = true;
      if (mfrc522.uid.sak == 0x00) {
        Serial.println("Ultralight detected");
        //RequestAuthUltralightCNetwork();
      } else if (mfrc522.uid.sak == 0x20) {
        Serial.println("Desfire detected");
        Desfire desfire = Desfire(&mfrc522,ip);
        Serial.println(mfrc522.uid.sak);
        dumpInfo(mfrc522.uid.uidByte, 10);
        boolean AndroidBool = true;
        if (AndroidBool) {
          Android android = Android(&mfrc522,ip);
          Buffer<7> buffer;
          byte aid[] = {0xF0, 0x39, 0x41, 0x45, 0x32, 0x81, 0x00};
          buffer.appendBuffer(aid, 7);

          if (!android.SelectApplication(buffer)) {
            Serial.println("Android Error");
            return;
          }
          if (!android.GetKey()) {
            return;
          }
          if (!android.Verify()) {
            return;
          }
        }

        boolean Reader = false;
        if (Reader) {
          uint32_t appId = desfire.GetAppIdFromNetwork();
          if (appId == 0) {
            return;
          }
          if (!desfire.SelectApplication(appId)) {
            return;
          }
          if (!desfire.AuthenticateNetwork(KEYTYPE_AES, 0)) {
            return;
          }
          Serial.println("Open door");
        }

        boolean Writer = false;
        if (Writer) {
          uint32_t appId = 0;
          KeySettings keySettings;
          if (!desfire.GetKeySettings(&keySettings)) {
            return;
          }
          KeyType masterKeyType = keySettings.keyType;
          if (!desfire.AuthenticateNetwork(masterKeyType, 0)) {
            //TODO check if applications can be created
          } else {
            if (masterKeyType != KEYTYPE_AES) {
              if (!desfire.ChangeKeyNetwork(KEYTYPE_AES)) {
                // should not happen
              }
            }
            appId = desfire.GetAppIdFromNetwork();
            if (appId == 0) {
              //Website neue Karte
            } else {
              if (!desfire.SelectApplication(appId)) {
                //Should not happen
              }
              if (!desfire.AuthenticateNetwork(KEYTYPE_AES, 0)) {
                appId = 0;
              } else {
                //KartenName auf Website anzeigen;
              }
            }
          }
        }
      } else {
        Serial.println(F("Other Card found, not compatible!"));
        mfrc522.PICC_HaltA();
        isPresent = false;
        return;
      }
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

void RequestAuthUltralightCNetwork() {
  Serial.println("Ultralight Auth");

  NetworkClient client(ip);
  byte AuthBuffer[24] = {0};  //
  byte AuthLength = 24;
  byte message[24] = {0};  // Message to transfer

  byte deviceCode[1] = {0};
  client.Send(deviceCode, 1);

  //#Step 0: Get and send id
  client.Send(mfrc522.uid.uidByte, 7);
  // Start Authentification
  // Step - 1
  //  Build command buffer
  AuthBuffer[0] = 0x1A;  // CMD_3DES_AUTH -> Ultralight C 3DES Authentication.
  AuthBuffer[1] = 0x00;  //

  // Calculate CRC_A
  status = mfrc522.PCD_CalculateCRC(AuthBuffer, 2, &AuthBuffer[2]);
  if (status != MFRC522::STATUS_OK) {
    return;
  }

  AuthLength = sizeof(AuthBuffer);

  // Transmit the buffer and receive the response, validate CRC_A.
  // Step - 2
  status = mfrc522.PCD_TransceiveData(AuthBuffer, 4, AuthBuffer, &AuthLength, nullptr, 0, true);
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Ultralight C Auth failed");
    Serial.println(MFRC522::GetStatusCodeName(status));
    Serial.print(F("Reply: "));
    dumpInfo(AuthBuffer, AuthLength);
    return;
  }
  memcpy(message, AuthBuffer + 1, 8);  // copy the enc(RndB) from the message
  // Step - 3
  client.Send(message, 8);  // ek(RndB)
  dumpInfo(message, 8);
  client.Recieve(message, 16);  // ek(RndA || RndB')
  dumpInfo(message, 16);
  AuthBuffer[0] = 0xAF;
  memcpy(AuthBuffer + 1, message, 16);  // copy the enc(RndB) from the message
  status = mfrc522.PCD_CalculateCRC(AuthBuffer, 17, &AuthBuffer[17]);
  if (status != MFRC522::STATUS_OK) {
    return;
  }
  // Step - 4
  status = mfrc522.PCD_TransceiveData(AuthBuffer, 19, AuthBuffer, &AuthLength, nullptr, 0, true);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Auth failed failed: "));
    Serial.println(MFRC522::GetStatusCodeName(status));
    Serial.println(F("Reply: "));
    dumpInfo(AuthBuffer, AuthLength);
    return;
  } else {
    if (AuthBuffer[0] == 0x00) {             // reply from PICC should start with 0x00
      memcpy(message, &AuthBuffer[1], 8);  // copy enc(RndA')
      client.Send(message, 8);             // ek(RndA')
      dumpInfo(message, 8);
    } else {
      Serial.println(F("Wrong answer!!!"));
    }
  }
}

uint32_t getNextFreeAppId(uint32_t appIds[], int length) {
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
