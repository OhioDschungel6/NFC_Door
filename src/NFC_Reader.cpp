/*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing how to read and modify data from a Mifare Ultralight C PICC
   --------------------------------------------------------------------------------------------------------------------

   This code enables basic communication with a Mifare Ultralight C PICC.
   It allows reading and writing of data including basic operations for password protection.

   This sketch imlements NO measures for page blocking and using OTP and the upwarts counter

   This sketch should run stable on a Arduinno Uno with RC522 module.
   But the code itself is very sensitive for modifications due to my bad programming
   or stability issues with the used libraries. If you change / optimize the code, be
   prepared for some weird complications

   have fun!

   @license Released into the public domain.

   use
   - auth 49454D4B41455242214E4143554F5946    for authentication (with standard key)
   - newKey 49454D4B41455242214E4143554F5946  for writing a new key (in this case the standard key) to the PICC.
   - dunp                                     to list the content of the PICC (data might not visible due to read-protection)
   - wchar 10 hello world                     to write "hello world" starting at page 10
   - whex 10 0123456789ABC                    to write HEX values starting at page 10
   - protect 25                               to protect the page from 25 upward
   - setpbit 1                                to set the protection to write-protected


   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

/*
   IMPORTANT!!!
   in order to be able to read Mifare Ultralight C PICC the typical RC522-module needs to be modified.
   L1 and L2 must be exchanged for better inductors (e.g. FERROCORE CW1008-2200). Otherwise this code will not work!
*/

/*
    The Mifare Ultralight C encrypted authentification process can be found here: https://www.nxp.com/docs/en/data-sheet/MF0ICU2.pdf
*/
#include <Arduino.h>
#include "NFC_Reader.h"
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN          21         // Configurable, see typical pin layout above
#define SS_PIN           5         // Configurable, see typical pin layout above


//Network
#include "NetworkManager.h"
NetworkManager networkManager;
#include "NetworkClient.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;

boolean isPresent = 0;
boolean isEncrypted = 0;
boolean isUlC = 0;
char Scmd[28];
char Sdata[64];

// byte page=0;

byte buffer[24];
boolean isLocked[255];                                // which Pages are locked to read only
byte startProtect = 255;                              // starting from which page, the data is protectet by the key
boolean access1;                                       // how is the data protected: 0=write protection (data visible) 1=read/write protection (data = 0x00)


void setup() {
  Serial.begin(115200);                               // Initialize serial communications with the PC
  while (!Serial);// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  networkManager.Setup();
  SPI.begin();                                        // Init SPI bus
  mfrc522.PCD_Init();                                 // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();                  // Show details of PCD - MFRC522 Card Reader details
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
    }
    else {
      isPresent = 1;
      if (mfrc522.uid.sak == 0x00) {
        dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
        RequestAuthUltralightCNetwork();
      } else if (mfrc522.uid.sak == 0x20) {
        dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
        RequestAuthDesfireNetwork();
      } else {
        Serial.println (F("Other Card found, not compatible!"));
        mfrc522.PICC_HaltA();
        isPresent = 0;
        return;
      }
    }
  }
  if (isPresent) {                            // test read - it it fails, the PICC is most likely gone
    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(0, buffer, &byteCount);
    if (status != mfrc522.STATUS_OK) {
      isPresent = 0;
      isEncrypted = 0;
      mfrc522.PCD_StopCrypto1();
      Serial.println("Card gone...");
      return;
    }
    
  }

}


void RequestAuthUltralightCNetwork(void) {

  NetworkClient client;
  byte AuthBuffer[24] = {0};            //
  byte AuthLength = 24;
  byte message[24] = {0};             // Message to transfer
  byte byteCount = sizeof(buffer);

  if (isEncrypted) {
    Serial.println(F("PICC already authenticated"));
    return;
  }
  //#Step 0: Get and send id
  client.Send(mfrc522.uid.uidByte, 7);
  //Start Authentification
  //Step - 1
  // Build command buffer
  AuthBuffer[0] = 0x1A; // CMD_3DES_AUTH -> Ultralight C 3DES Authentication.
  AuthBuffer[1] = 0x00; //

  // Calculate CRC_A
  status = mfrc522.PCD_CalculateCRC(AuthBuffer, 2, &AuthBuffer[2]);
  if (status != mfrc522.STATUS_OK) {
    return;
  }

  AuthLength = sizeof(AuthBuffer);

  // Transmit the buffer and receive the response, validate CRC_A.
  //Step - 2
  status = mfrc522.PCD_TransceiveData(AuthBuffer, 4, AuthBuffer, &AuthLength, NULL, 0, true);
  if (status != mfrc522.STATUS_OK) {
    Serial.println("Ultralight C Auth failed");
    Serial.println(mfrc522.GetStatusCodeName(status));
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
  if (status != mfrc522.STATUS_OK) {
    return;
  }
  //Step - 4
  status = mfrc522.PCD_TransceiveData(AuthBuffer, 19, AuthBuffer, &AuthLength, NULL, 0, true);
  if (status != mfrc522.STATUS_OK) {
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
  return;
}

void RequestAuthDesfireNetwork(void) {

  NetworkClient client;
  byte AuthBuffer[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //
  byte AuthLength = 24;
  byte message[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Message to transfer
  byte byteCount = sizeof(buffer);

  if (isEncrypted) {
    Serial.println(F("PICC already authenticated"));
    return;
  }
  //#Step 0: Get and send id
  client.Send(mfrc522.uid.uidByte, 7);
  //Start Authentification
  //Step - 1
  // Build command buffer
  AuthBuffer[0] = 0x1A; // CMD_3DES_AUTH -> Ultralight C 3DES Authentication.
  AuthBuffer[1] = 0x00; //

  // Calculate CRC_A
  status = mfrc522.PCD_CalculateCRC(AuthBuffer, 2, &AuthBuffer[2]);
  if (status != mfrc522.STATUS_OK) {
    return;
  }

  AuthLength = sizeof(AuthBuffer);

  // Transmit the buffer and receive the response, validate CRC_A.
  //Step - 2
  status = mfrc522.PCD_TransceiveData(AuthBuffer, 4, AuthBuffer, &AuthLength, NULL, 0, true);
  if (status != mfrc522.STATUS_OK) {
    Serial.println("Ultralight C Auth failed");
    Serial.println(mfrc522.GetStatusCodeName(status));
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
  if (status != mfrc522.STATUS_OK) {
    return;
  }
  //Step - 4
  status = mfrc522.PCD_TransceiveData(AuthBuffer, 19, AuthBuffer, &AuthLength, NULL, 0, true);
  if (status != mfrc522.STATUS_OK) {
    Serial.print(F("Auth failed failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    Serial.println(F("Reply: "));
    dumpInfo(AuthBuffer, AuthLength);
    return;
  }
  else {
    if (AuthBuffer[0] == 0x00) {                        // reply from PICC should start with 0x00
      memcpy(message, &AuthBuffer[1], 8);               // copy enc(RndA')
      client.Send(message, 8); //ek(RndA')
      dumpInfo(message, 8);
    }
    else {
      Serial.println(F("Wrong answer!!!"));
    }
  }
  return;
}



boolean checkIfUltralight(void) {
  byte Count = sizeof(buffer);

  if (mfrc522.MIFARE_Read(43, buffer, &Count) == mfrc522.STATUS_OK) {
    if (mfrc522.MIFARE_Read(44, buffer, &Count) == mfrc522.STATUS_OK) {
      return 0;
    }
    else {
      Count = sizeof(buffer);
      status = mfrc522.PICC_WakeupA(buffer, &Count);          // needed to wake up the card after receiving a NAK-answer
      return 1;
    }
  }
  Count = sizeof(buffer);
  status = mfrc522.PICC_WakeupA(buffer, &Count);              // needed to wake up the card after receiving a NAK-answer
  Serial.println(mfrc522.GetStatusCodeName(status));
  return 0;
}

void setBooleanBits(boolean *ar, int len) {
  for (int i = 0; i < len; i++)
    ar[i] = 1;
}

void writeData(boolean mode) {
  byte page = 0;
  byte i = 0;
  byte Buffer[4] = {0, 0, 0, 0};

  if (isUlC)
    Serial.println(F("Ultralight C"));
  else
    Serial.println(F("not a Ultralight C PICC!"));

  while ((Sdata[i] != ' ') && (i < 3)) {
    page = page * 10;
    page = page + Sdata[i] - 48;
    i++;
  }
  i++;                                  // to overcome the ' '

  if (page < 4) {
    Serial.println(F("no user memory here"));
    return;
  }
  if (isUlC && (page > 39)) {
    Serial.println(F("no user memory here"));
    return;
  }
  byte j = 0;
  boolean done = 0;
  for (; page < 40; page++) {
    for (j = 0; j < 4; j++) {
      if (Sdata[i] == 0x00) {
        while (j < 4) {
          Buffer[j] = 0;
          j++;
        }
        done = 1;
      }
      else {
        if (mode) {
          Buffer[j] = char2byte(Sdata + i);
          i = i + 2;
        }
        else {
          Buffer[j] = Sdata[i];
          i++;
        }
      }
    }

    status = mfrc522.MIFARE_Ultralight_Write(page, Buffer, 4);
    Serial.print(F("writing page "));
    Serial.print(page);
    Serial.println(mfrc522.GetStatusCodeName(status));
    if (status != mfrc522.STATUS_OK) {
      return;
    }
    if (done)
      return;
  }
}



// Needed to create RndB' out of RndB
void rol(byte *data, int len) {
  byte first = data[0];
  for (int i = 0; i < len - 1; i++) {
    data[i] = data[i + 1];
  }
  data[len - 1] = first;
}



void dumpInfo(byte *ar, int len) {
  for (int i = 0 ; i < len ; i++) {
    if (ar[i] < 0x10)
      Serial.print(F("0"));
    Serial.print(ar[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println("");
}



byte char2byte(char *s) {
  byte x = 0;
  for (int i = 0; i < 2 ; i++) {
    char c = *s;
    if (c >= '0' && c <= '9') {
      x *= 16;
      x += c - '0';
    }
    else if (c >= 'A' && c <= 'F') {
      x *= 16;
      x += (c - 'A') + 10;
    }
    else if (c >= 'a' && c <= 'f') {
      x *= 16;
      x += (c - 'a') + 10;
    }
    s++;
  }
  return x;
}


/*
   If the hex key is: "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F"
   then you have to write the sequence "07 06 05 04 03 02 01 00 0F 0E 0D 0C 0B 0A 09 08" in 4 pages,
   from page 0x2C (44) to page 0x2F (47).

   if you want to change where (which pages) the authentication is required, here is how to do that:
   0x2A defines the page address from which the authentication is required. E.g. if 0x2A = 0x30 no authentication is needed all as memory goes up to 0x2F.
   0x2B defines if authentication is required for read/write (0x2B=0) or only for write access1 (0x2B=1)

  On example of Key1 = 0001020304050607h and Key2 = 08090A0B0C0D0E0Fh,
  the command sequence needed for key programming with WRITE command is:
  •A2 2C 07 06 05 04 CRC
  •A2 2D 03 02 01 00 CRC
  •A2 2E 0F 0E 0D 0C CRC
  •A2 2F 0B 0A 09 08 CRC

*/
