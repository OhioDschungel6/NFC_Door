
#include <Arduino.h>
#include "NFC_Reader.h"
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

NetworkManager networkManager;

#include "NetworkClient.h"

//MFRC522DriverPinSimple ss_pin(SS_PIN); // Configurable, see typical pin layout above.
//MFRC522DriverSPI driver{ss_pin}; // Create SPI driver.
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
            if (mfrc522.uid.sak == 0x00) {
                dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
                RequestAuthUltralightCNetwork();
            } else if (mfrc522.uid.sak == 0x20) {
                dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
                RequestAuthDesfireNetwork();
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

    NetworkClient client;
    byte AuthBuffer[24] = {0};            //
    byte AuthLength = 24;
    byte message[24] = {0};             // Message to transfer
    byte byteCount = sizeof(buffer);


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

void RequestAuthDesfireNetwork() {

    NetworkClient client;
    byte AuthBuffer[128] = {0}; //
    byte AuthLength = 128;
    byte message[128] = {0}; // Message to transfer
    byte byteCount = sizeof(buffer);

    //#Step 0: Get and send id
    client.Send(mfrc522.uid.uidByte, 7);
    //Start Authentification
    //Step - 1
    // Build command buffer
    AuthBuffer[0] = 0xAA; //

    AuthLength = sizeof(AuthBuffer);

    // Transmit the buffer and receive the response, validate CRC_A.
    //Step - 2

    MFRC522::Uid uid;
    mfrc522.PICC_Select(&uid, 0);
    mfrc522.TCL_Transceive(&mfrc522.tag,AuthBuffer,2,AuthBuffer,&AuthLength);
//    status = mfrc522.PCD_TransceiveData14443_4(MFRC522Constants::DESFIRE_Auth,AuthBuffer,2, AuthBuffer, &AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Desfire Auth failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        Serial.print(F("Reply: "));
        dumpInfo(AuthBuffer, AuthLength);
        return;
    }
    memcpy(message, AuthBuffer + 1, 16); // copy the enc(RndB) from the message
    dumpInfo(AuthBuffer,20);
    //Step - 3
    client.Send(message, 16); //ek(RndB)
    dumpInfo(message, 16);
    client.Recieve(message, 32); // ek(RndA || RndB')
    dumpInfo(message, 32);
    AuthBuffer[0] = 0xAF;
    memcpy(AuthBuffer + 1, message, 32); // copy the enc(RndB) from the message
    status = mfrc522.PCD_CalculateCRC(AuthBuffer, 17, &AuthBuffer[33]);
    if (status != MFRC522::STATUS_OK) {
        return;
    }
    //Step - 4
    status = mfrc522.PCD_TransceiveData(AuthBuffer, 19, AuthBuffer, &AuthLength, nullptr, 0, true);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Auth failed failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
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

void dumpInfo(byte *ar, int len) {
    for (int i = 0; i < len; i++) {
        if (ar[i] < 0x10)
            Serial.print(F("0"));
        Serial.print(ar[i], HEX);
        Serial.print(F(" "));
    }
    Serial.println("");
}