
#include <Arduino.h>
//#include "NFC_Reader.h"
#include <SPI.h>
//#include <MFRC522v2.h>
//#include <MFRC522DriverPinSimple.h>
//#include <MFRC522DriverSPI.h>
//#include <MFRC522Debug.h>
#include <MFRC522Extended.h>

#define RST_PIN          21         // Configurable, see typical pin layout above
#define SS_PIN           5         // Configurable, see typical pin layout above


//Network
#include "NetworkManager.h"
#include "Utils.h"

NetworkManager networkManager;

#include "NetworkClient.h"
#include "Desfire.h"

MFRC522Extended mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;

boolean isPresent = false;

byte buffer[24];

void setup() {
    Serial.begin(115200);
    while (!Serial);
    networkManager.Setup();
    SPI.begin();                                        // Init SPI bus
    mfrc522.PCD_Init();                                 // Init MFRC522
//    MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
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
            Serial.println("Sak:");
            Serial.println(mfrc522.uid.sak);
            if (mfrc522.uid.sak == 0x00) {
              Serial.println("Ultralight detected");
                //dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
                RequestAuthUltralightCNetwork();
            } else if (mfrc522.uid.sak == 0x20) {
              Serial.println("Desfire detected");
              Desfire desfire = Desfire(&mfrc522);
              if( desfire.AuthenticateNetwork(KEYTYPE_3DES,0)){
                
              }
            } else {
                Serial.println(F("Other Card found, not compatible!"));
                mfrc522.PICC_HaltA();
                isPresent = false;
                return;
            }
        }
    }
    if (isPresent) {                            // test read - it it fails, the PICC is most likely gone
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
    byte AuthBuffer[24] = {0};            //
    byte AuthLength = 24;
    byte message[24] = {0};             // Message to transfer

    byte deviceCode[1] = {0};
    client.Send(deviceCode,1);

    //#Step 0: Get and send id
    client.Send(mfrc522.uid.uidByte, 7);
    //Start Authentification
    //Step - 1
    // Build command buffer
    AuthBuffer[0] = 0x1A; // CMD_3DES_AUTH -> Ultralight C 3DES Authentication.
    AuthBuffer[1] = 0x00; //

    // Calculate CRC_A
    status = mfrc522.PCD_CalculateCRC(AuthBuffer, 2, &AuthBuffer[2]);
    if (status != MFRC522::STATUS_OK) {
        return;
    }

    AuthLength = sizeof(AuthBuffer);

    // Transmit the buffer and receive the response, validate CRC_A.
    //Step - 2
    status = mfrc522.PCD_TransceiveData(AuthBuffer, 4, AuthBuffer, &AuthLength, nullptr, 0, true);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Ultralight C Auth failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        Serial.print(F("Reply: "));
        dumpInfo(AuthBuffer, AuthLength);
        return;
    }
    memcpy(message, AuthBuffer + 1, 8); // copy the enc(RndB) from the message
    //Step - 3
    client.Send(message, 8); //ek(RndB)
    dumpInfo(message, 8);
    client.Recieve(message, 16); // ek(RndA || RndB')
    dumpInfo(message, 16);
    AuthBuffer[0] = 0xAF;
    memcpy(AuthBuffer + 1, message, 16); // copy the enc(RndB) from the message
    status = mfrc522.PCD_CalculateCRC(AuthBuffer, 17, &AuthBuffer[17]);
    if (status != MFRC522::STATUS_OK) {
        return;
    }
    //Step - 4
    status = mfrc522.PCD_TransceiveData(AuthBuffer, 19, AuthBuffer, &AuthLength, nullptr, 0, true);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Auth failed failed: "));
        Serial.println(MFRC522::GetStatusCodeName(status));
        Serial.println(F("Reply: "));
        dumpInfo(AuthBuffer, AuthLength);
        return;
    } else {
        if (AuthBuffer[0] == 0x00) {                        // reply from PICC should start with 0x00
            memcpy(message, &AuthBuffer[1], 8);               // copy enc(RndA')
            client.Send(message, 8); //ek(RndA')
            dumpInfo(message, 8);
        } else {
            Serial.println(F("Wrong answer!!!"));
        }
    }
}

void DesfireAuth3Des() {

    Serial.println("Desfire 3Des Auth");

    NetworkClient client;
    byte AuthBuffer[24] = {0};            //
    byte AuthLength = 24;
    byte message[24] = {0};             // Message to transfer

    byte deviceCode[1] = {0};
    client.Send(deviceCode,1);

    //#Step 0: Get and send id
    client.Send(mfrc522.uid.uidByte, 7);
    //Start Authentification
    //Step - 1
    // Build command buffer
    AuthBuffer[0] = 0x1A; // CMD_3DES_AUTH
    AuthBuffer[1] = 0x00; //

    // Calculate CRC_A
    status = mfrc522.PCD_CalculateCRC(AuthBuffer, 2, &AuthBuffer[2]);
    if (status != MFRC522::STATUS_OK) {
        return;
    }

    // Transmit the buffer and receive the response, validate CRC_A.
    //Step - 2
    status = mfrc522.TCL_Transceive(&mfrc522.tag,AuthBuffer,2,AuthBuffer,&AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println(MFRC522::GetStatusCodeName(status));
        return;
    }
    Serial.println("first receive");
    dumpInfo(AuthBuffer, AuthLength);
    memcpy(message, AuthBuffer + 1, 8); // copy the enc(RndB) from the message
    //Step - 3
    client.Send(message, 8); //ek(RndB)
    dumpInfo(message, 8);
    client.Recieve(message, 16); // ek(RndA || RndB')
    dumpInfo(message, 16);
    AuthBuffer[0] = 0xAF;
    memcpy(AuthBuffer + 1, message, 16); // copy the enc(RndB) from the message
    status = mfrc522.PCD_CalculateCRC(AuthBuffer, 17, &AuthBuffer[17]);
    if (status != MFRC522::STATUS_OK) {
        return;
    }
    //Step - 4
    status = mfrc522.TCL_Transceive(&mfrc522.tag,AuthBuffer,17,AuthBuffer,&AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Auth failed failed: "));
        Serial.println(MFRC522::GetStatusCodeName(status));
        Serial.println(F("Reply: "));
        dumpInfo(AuthBuffer, AuthLength);
        return;
    } else {
         dumpInfo(AuthBuffer, AuthLength);
        if (AuthBuffer[0] == 0x00) {                        // reply from PICC should start with 0x00
            memcpy(message, &AuthBuffer[1], 8);               // copy enc(RndA')
            client.Send(message, 8); //ek(RndA')
            dumpInfo(message, 8);
            Serial.println("Auth succesful");
        } else {
            Serial.println(F("Wrong answer!!!"));
        }
    }
}

void TestDesfire(){
    Serial.println("TestDesfire start");
    byte AuthBuffer[128] = {0}; //
    byte AuthLength = 128;
    byte message[128] = {0}; 

    //Maybe first select application
    //0x00AE16
    
    //--------------------------------------------------
    AuthBuffer[0] = 0xAA; // Cmd Authentificate
    //AuthBuffer[0] = 0x1A; // 3DES Authentificate
    AuthBuffer[1] = 0x01; // Masterkey
    byte msgLength = 2;
  
    status = mfrc522.PCD_CalculateCRC(AuthBuffer, 2, &AuthBuffer[2]);
    if (status != MFRC522::STATUS_OK) {
        return;
    }
    Serial.println("CRC done");
    status = mfrc522.TCL_Transceive(&mfrc522.tag,AuthBuffer,2,AuthBuffer,&AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Desfire Auth failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return;
    }
    dumpInfo(AuthBuffer,AuthLength);
    Serial.println("Desfire Auth good lel");
}

void RequestAuthDesfireNetwork() {

    NetworkClient client;
    byte AuthBuffer[128] = {0}; //
    byte AuthLength = 128;
    byte message[128] = {0}; // Message to transfer
    byte byteCount = sizeof(buffer);

    byte deviceCode[1] = {1};
    client.Send(deviceCode,1);

    //#Step 0: Get and send id
    client.Send(mfrc522.uid.uidByte, 7);
    //Start Authentification
    //Step - 1
    // Build command buffer
    AuthBuffer[0] = 0xAA; //
    //AuthBuffer[0] = 0x71; //
    AuthBuffer[1] = 0x00; // 1st Key
    //AuthBuffer[2] = 0x00; // LenCap
    

    // Transmit the buffer and receive the response, validate CRC_A.
    //Step - 2
    status = mfrc522.PCD_CalculateCRC(AuthBuffer, 2, &AuthBuffer[2]);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("CRC not OK");
        return;
    }
    status = mfrc522.TCL_Transceive(&mfrc522.tag,AuthBuffer,2,AuthBuffer,&AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Desfire Auth failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return;
    }
    dumpInfo(AuthBuffer, AuthLength);
    memcpy(message, AuthBuffer + 1, 16); // copy the enc(RndB) from the message
    //dumpInfo(AuthBuffer,17);
    
    //Step - 3
    client.Send(message, 16); //ek(RndB)
    dumpInfo(message, 16);
    client.Recieve(message, 32); // ek(RndA || RndB')
    dumpInfo(message, 32);
    AuthBuffer[0] = 0xAF;
    memcpy(AuthBuffer + 1, message, 32); // copy the enc(RndB) from the message Comment is trash
    status = mfrc522.PCD_CalculateCRC(AuthBuffer, 33, &AuthBuffer[33]);
    if (status != MFRC522::STATUS_OK) {
        return;
    }
    //Step - 4
    status =  mfrc522.TCL_Transceive(&mfrc522.tag,AuthBuffer,33,AuthBuffer,&AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Auth failed failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        Serial.println(F("Reply: "));
        dumpInfo(AuthBuffer, AuthLength);
        return;
    } else {
        if (AuthBuffer[0] == 0x00) {                        // reply from PICC should start with 0x00
            memcpy(message, &AuthBuffer[1], 16);               // copy enc(RndA')
            client.Send(message, 16); //ek(RndA')
            dumpInfo(message, 16);
        } else {
            Serial.println(F("Wrong answer!!!"));
            dumpInfo(AuthBuffer, AuthLength);
        }
    }
}
