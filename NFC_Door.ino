
#include <Arduino.h>
#include <MFRC522Extended.h>
#include <SPI.h>
#include "NetworkManager.h"
#include "Utils.h"
#include "Desfire.h"
#include "NetworkClient.h"

#define RST_PIN 21 
#define SS_PIN 5 

NetworkManager networkManager;
MFRC522Extended mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;

boolean isPresent = false;


void setup() {
    Serial.begin(115200);
    while (!Serial) ;
    networkManager.Setup();
    SPI.begin();        
    mfrc522.PCD_Init();  
    Serial.println(F("Setup ready"));
}

void loop() {
    delay(3000);
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
                RequestAuthUltralightCNetwork();
            } else if (mfrc522.uid.sak == 0x20) {
                Serial.println("Desfire detected");
                Desfire desfire = Desfire(&mfrc522);            

                boolean Reader = false;
                if(Reader){
                  uint32_t appId = desfire.GetAppIdFromNetwork();         
                  if( appId == 0){
                    return;
                  }
                  if( !desfire.SelectApplication(appId)){
                     return;
                  }
                  if( !desfire.AuthenticateNetwork(KEYTYPE_AES,0)){
                     return;
                  }
                  Serial.println("Open door");
               }
               
               boolean Writer = true;
                if(Writer){
                  KeySettings keySettings;
                  if(!desfire.GetKeySettings(&keySettings)){
                    return;
                  }
                  KeyType masterKeyType = keySettings.keyType;
                  if( !desfire.AuthenticateNetwork(masterKeyType,0)){
                    //TODO check if applications can be created
                  } else{
                    if(masterKeyType != KEYTYPE_AES){
                      if(!desfire.ChangeKeyNetwork(KEYTYPE_AES)){
                        // should not happen
                      }
                    }
                    uint32_t appId = desfire.GetAppIdFromNetwork();         
                    if( appId == 0){
                      //Website neue Karte
                    }else{
                        if( !desfire.SelectApplication(appId)){
                          //Should not happen
                        }
                        if( !desfire.AuthenticateNetwork(KEYTYPE_AES,0)){
                           appId = 0;
                        }else{
                           //KartenName auf Website anzeigen;
                        }
                    }
                  }
                  //TODO Event write key from card
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
        byte buffer [20] = {0};
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

    NetworkClient client;
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
