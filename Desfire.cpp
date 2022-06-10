#include "Desfire.h"

#include <CRC32.h>

int getKeyLength(int keytype);
boolean encryptDataframe(byte dataframe[], int length);

Desfire::Desfire(MFRC522Extended* mfrc522) {
    this->mfrc522 = mfrc522;
}

boolean Desfire::AuthenticateNetwork(int keytype, int keyNr) {
    byte cmd;
    switch (keytype) {
        case KEYTYPE_2K3DES:
        case KEYTYPE_3DES:
            cmd = 0x1A;
            break;
        case KEYTYPE_AES:
            cmd = 0xAA;
            break;
        default:
            // Not implemented
            return false;
    }
    NetworkClient client;
    byte AuthBuffer[64] = {0};  //
    byte AuthLength = 64;
    byte message[64] = {0};  // Message to transfer

    byte authMode[1] = getAuthMode(keytype);
    byte blockSize = getAuthBlockSize(keytype);

    // Notify server if 3DES or AES
    client.Send(authMode, 1);

    //#Step 0: Get and send id
    client.Send(mfrc522->uid.uidByte, 7);

    // Start Authentification
    MFRC522::StatusCode status;
    // Step - 1
    //  Build command buffer
    AuthBuffer[0] = cmd;
    AuthBuffer[1] = keyNr;

    // Transmit the buffer and receive the response
    // Step - 2
    status = mfrc522->TCL_Transceive(&mfrc522->tag, AuthBuffer, 2, AuthBuffer, &AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Desfire Auth failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (AuthBuffer[0] != DesfireStatusCode_ADDITIONAL_FRAME) {
        Serial.println("Desfire Auth failed");
        // TODO ERROR CODE
        return false;
    }

    memcpy(message, AuthBuffer + 1, blockSize);  // copy the enc(RndB) from the message

    // Step - 3
    client.Send(message, blockSize);         // ek(RndB)
    client.Recieve(message, blockSize * 2);  // ek(RndA || RndB')
    AuthBuffer[0] = DesfireStatusCode_ADDITIONAL_FRAME;
    memcpy(AuthBuffer + 1, message, blockSize * 2);
    // Step - 4
    status = mfrc522->TCL_Transceive(&mfrc522->tag, AuthBuffer, 1 + 2 * blockSize, AuthBuffer, &AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Auth failed failed: "));
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    } else {
        if (AuthBuffer[0] == DesfireStatusCode_OPERATION_OK) {  // reply from PICC should start with 0x00
            memcpy(message, &AuthBuffer[1], 16);                // copy enc(RndA')
            client.Send(message, 16);                           // ek(RndA')
            AuthType = keytype;
            authenticated = true;
            authkeyNr = keyNr;
            if (AuthType == KEYTYPE_2K3DES) {
                client.Recieve(sessionKey, 16);
                byte deskey[24];  // = {0x36 ,0xC4 ,0xF8 ,0xBE ,0x30 ,0x6E ,0x6C ,0x76 ,0xAC ,0x22 ,0x9E ,0x8C ,0xF8 ,0x24 ,0xBA ,0x30 ,0x32 ,0x50 ,0xD4 ,0xAA ,0x64 ,0x36 ,0x56 ,0xA2};
                memcpy(deskey, sessionKey, 16);
                memcpy(deskey + 16, sessionKey, 8);
                des.init(deskey, 0);
            } else if (AuthType == KEYTYPE_3DES) {
                client.Recieve(sessionKey, 24);
                byte deskey[24];  // = {0x36 ,0xC4 ,0xF8 ,0xBE ,0x30 ,0x6E ,0x6C ,0x76 ,0xAC ,0x22 ,0x9E ,0x8C ,0xF8 ,0x24 ,0xBA ,0x30 ,0x32 ,0x50 ,0xD4 ,0xAA ,0x64 ,0x36 ,0x56 ,0xA2};
                memcpy(deskey, sessionKey, 24);
                des.init(deskey, 0);
            } else if (AuthType == KEYTYPE_AES) {
                client.Recieve(sessionKey, 16);
                byte iv[16] = {0};
                aes.setIV(iv);
                aes.setKey(sessionKey, 128);
            }
            Serial.println("Auth succesful");
            return true;
        } else {
            Serial.println(F("Wrong answer!"));
            dumpInfo(AuthBuffer, AuthLength);
        }
    }
    return false;
}

boolean Desfire::SelectApplication(uint32_t appId) {
    Serial.println("Select Application");
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.append(0x5A);
    message.append24(appId);

    byte response[32] = {0};
    int responseLength;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Select App failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (response[0] != DesfireStatusCode_OPERATION_OK) {
        Serial.println("Select App failed");
        return false;
    }
    applicationNr = appId;
    Serial.println("App selected");
    return true;
}
boolean Desfire::DeleteApplication(uint32_t appId) {
    Serial.println("Delete Application");
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.append(0xDA);
    message.append24(appId);

    byte response[32] = {0};
    int responseLength;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Delete App failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (response[0] != DesfireStatusCode_OPERATION_OK) {
        Serial.println("Delete App failed");
        return false;
    }
    Serial.println("App deleted");
    return true;
}

boolean Desfire::FormatCard() {
    Serial.println("Format card");
    MFRC522::StatusCode status;
    byte message[32] = {0};
    byte messageLength = 32;
    message[0] = 0xFC;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message, 1, message, &messageLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Formatting failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (message[0] != DesfireStatusCode_OPERATION_OK) {
        Serial.println("Formatting failed");
        dumpInfo(message, messageLength);
        return false;
    }
    Serial.println("Formatting succesful");
    return true;
}

boolean Desfire::CreateApplication(uint32_t appId, byte keyCount, int keyType) {
    Serial.println("Create Application");
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.append(0xCA);
    message.append24(appId);
    message.append(0x0F);  // Who can change key, 0F is factory default. TODO enum;
    message.append(keyType | keyCount);
    // dumpInfo(message, 6);

    byte response[32] = {0};
    int responseLength;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Create App failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (response[0] != DesfireStatusCode_OPERATION_OK) {
        Serial.println("Create App failed");
        dumpInfo(response, responseLength);
        return false;
    }
    Serial.println("Application created");
    return true;
}

boolean Desfire::ChangeKey(byte key[], int keytype, int keyNr) {
    if (!authenticated) {
        Serial.println("Not authenticated");
        return false;
    }
    int keyLength = getKeyLength(keytype);
    boolean isSameKey = (keyNr == authkeyNr);

    byte keyVersion = 1;
    byte message[40] = {0};
    byte messageLength = 0;

    message[0] = 0xC4;
    message[1] = keyNr;
    memcpy(message + 2, key, keyLength);
    if (applicationNr == 0) {
        message[1] |= keytype;
    }
    if (!isSameKey) {
        byte oldKey[24] = {0};
        for (int i = 0; i < keyLength; i++) {
            message[i + 2] ^= oldKey[i];
        }
    }
    messageLength = keyLength + 2;
    if (keytype == KEYTYPE_AES) {
        message[messageLength] = keyVersion;
        messageLength++;
    }

    // Calculate CRC32
    uint32_t crc32 = ~CRC32::calculate(message, messageLength);
    message[messageLength] = (crc32)&0xFF;
    message[messageLength + 1] = (crc32 >> 8) & 0xFF;
    message[messageLength + 2] = (crc32 >> 16) & 0xFF;
    message[messageLength + 3] = crc32 >> 24;
    messageLength += 4;
    if (!isSameKey) {
        crc32 = ~CRC32::calculate(key, keyLength);
        message[messageLength] = (crc32)&0xFF;
        message[messageLength + 1] = (crc32 >> 8) & 0xFF;
        message[messageLength + 2] = (crc32 >> 16) & 0xFF;
        message[messageLength + 3] = crc32 >> 24;
        messageLength += 4;
    }
    // TODO only for applicationKeys atm
    int blockSize = getBlockSize(AuthType);
    if ((messageLength - 2) % blockSize != 0) {
        // setSize to next multiple of block size
        messageLength += blockSize - ((messageLength - 2) % blockSize);
    }
    // Encrypt
    byte encDataframe[messageLength] = {0};

    if (!EncryptDataframe(message + 2, encDataframe, messageLength - 2)) {
        Serial.println("Encryption failed");
        return false;
    }
    memcpy(message + 2, encDataframe, messageLength - 2);
    MFRC522::StatusCode status;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message, messageLength, message, &messageLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Key change failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (isSameKey) {
        authenticated = false;
    }
    if (message[0] != DesfireStatusCode_OPERATION_OK) {
        dumpInfo(message, messageLength);
        Serial.println("Key change failed");
        return false;
    }
    Serial.println("Key changed");
    return true;
}

int Desfire::GetAppIds(uint32_t appIds[], int maxLength) {
    MFRC522::StatusCode status;
    byte message[128] = {0};
    byte messageLength = 128;
    int ids = 0;
    message[0] = 0x6A;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message, 1, message, &messageLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Retrieving App Ids failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (message[0] != DesfireStatusCode_OPERATION_OK && message[0] != DesfireStatusCode_ADDITIONAL_FRAME) {
        Serial.println("Retrieving App Ids failed");
        dumpInfo(message, messageLength);
        return ids;
    }
    dumpInfo(message, messageLength);
    for (int i = 1; i <= messageLength - 3; i += 3) {
        appIds[ids++] = parseAppId(&p[i]);
        if (ids == maxLength) {
            return ids;
        }
    }
    while (message[0] == DesfireStatusCode_ADDITIONAL_FRAME) {
        message[0] = 0xAF;
        status = mfrc522->TCL_Transceive(&mfrc522->tag, message, 1, message, &messageLength);
        if (status != MFRC522::STATUS_OK) {
            Serial.println("Retrieving App Ids failed");
            Serial.println(MFRC522::GetStatusCodeName(status));
            return ids;
        }
        if (message[0] != DesfireStatusCode_OPERATION_OK && message[0] != DesfireStatusCode_ADDITIONAL_FRAME) {
            Serial.println("Retrieving App Ids failed");
            dumpInfo(message, messageLength);
            return ids;
        }
        for (int i = 1; i <= messageLength - 3; i += 3) {
            appIds[ids++] = parseAppId(&p[i]);
            if (ids == maxLength) {
                return ids;
            }
        }
    };
    return ids;
}

boolean Desfire::EncryptDataframe(byte dataframe[], byte encDataframe[], int length) {
    if (!authenticated) {
        Serial.println("Not authenticated");
        return false;
    }
    if (length % getBlockSize(AuthType) != 0) {
        Serial.println("Block size error");
        return false;
    }
    if (AuthType == KEYTYPE_3DES || AuthType == KEYTYPE_2K3DES) {
        des.set_size(length);
        des.tdesCbcEncipher(dataframe, encDataframe);
        Serial.println("Encrpyted");
        return true;
    } else if (AuthType == KEYTYPE_AES) {
        aes.encryptCBC(length, dataframe, encDataframe);
        return true;
    }
    return false;
}

int getKeyLength(KeyType keytype) {
    switch (keytype) {
        case KEYTYPE_2K3DES:
            return 16;
        case KEYTYPE_3DES:
            return 24;
        case KEYTYPE_AES:
            return 16;
        default:
            return 0;
    }
}

int getBlockSize(KeyType keytype) {
    switch (keytype) {
        case KEYTYPE_2K3DES:
            return 8;
        case KEYTYPE_3DES:
            return 8;
        case KEYTYPE_AES:
            return 16;
        default:
            return 0;
    }
}

int getAuthBlockSize(KeyType keytype) {
    switch (keytype) {
        case KEYTYPE_2K3DES:
            return 8;
        case KEYTYPE_3DES:
            return 16;
        case KEYTYPE_AES:
            return 16;
        default:
            return 0;
    }
}

byte getAuthMode(KeyType keytype) {
    switch (keytype) {
        case KEYTYPE_2K3DES:
            return 0;
        case KEYTYPE_3DES:
            return 1;
        case KEYTYPE_AES:
            return 2;
        default:
            return -1;
    }
}