#include "Desfire.h"

#include <CRC32.h>

int getKeyLength(KeyType keyType);
int getBlockSize(KeyType keyType);
int getAuthBlockSize(KeyType keyType);
byte getAuthMode(KeyType keyType);

Desfire::Desfire(MFRC522Extended* mfrc522,String ip) {
    this->mfrc522 = mfrc522;
    this->ip = ip;
}

boolean Desfire::AuthenticateNetwork(KeyType keyType, int keyNr) {
    DesfireCommand cmd;
    switch (keyType) {
        case KEYTYPE_2K3DES:
        case KEYTYPE_3DES:
            cmd = DesfireCommand_AUTHENTICATE_LEGACY;
            break;
        case KEYTYPE_AES:
            cmd = DesfireCommand_AUTHENTICATE;
            break;
        default:
            // Not implemented
            return false;
    }
    NetworkClient client(ip);
    Buffer<64> message;
    byte response[64] = {0};
    byte responseLength = 64;

    byte serverCommand = 0xAA;
    byte blockSize = getAuthBlockSize(keyType);

    // Send Auth command
    client.Send(&serverCommand, 1);
    byte kt = keyType;
    client.Send(&kt, 1);

    //#Step 0: Get and send id
    client.Send(mfrc522->uid.uidByte, 7);

    Buffer<3> buffer;
    buffer.append24(applicationNr);
    client.Send(buffer.buffer, 3);

    // Start Authentification
    MFRC522::StatusCode status;
    // Step - 1
    //  Build command buffer
    message.append(cmd);
    message.append(keyNr);

    // Transmit the buffer and receive the response
    // Step - 2
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Desfire Auth failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (response[0] != DesfireStatusCode_ADDITIONAL_FRAME) {
        Serial.println("Desfire Auth failed");
        dumpInfo(response, responseLength);
        return false;
    }

    // Step - 3
    client.Send(response + 1, blockSize);     // ek(RndB)
    client.Recieve(response, blockSize * 2);  // ek(RndA || RndB')

    message.clear();
    message.append(DesfireStatusCode_ADDITIONAL_FRAME);
    message.appendBuffer(response, blockSize * 2);
    // Step - 4
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Auth failed failed: "));
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    } else {
        if (response[0] == DesfireStatusCode_OPERATION_OK) {  // reply from PICC should start with 0x00
            client.Send(&response[1], blockSize);             // ek(RndA')
            authType = keyType;
            authenticated = true;
            authkeyNr = keyNr;
            // if (authType == KEYTYPE_2K3DES) {
            //     client.Recieve(sessionKey, 16);
            //     byte deskey[24];  // = {0x36 ,0xC4 ,0xF8 ,0xBE ,0x30 ,0x6E ,0x6C ,0x76 ,0xAC ,0x22 ,0x9E ,0x8C ,0xF8 ,0x24 ,0xBA ,0x30 ,0x32 ,0x50 ,0xD4 ,0xAA ,0x64 ,0x36 ,0x56 ,0xA2};
            //     memcpy(deskey, sessionKey, 16);
            //     memcpy(deskey + 16, sessionKey, 8);
            //     des.init(deskey, 0);
            // } else if (authType == KEYTYPE_3DES) {
            //     client.Recieve(sessionKey, 24);
            //     byte deskey[24];  // = {0x36 ,0xC4 ,0xF8 ,0xBE ,0x30 ,0x6E ,0x6C ,0x76 ,0xAC ,0x22 ,0x9E ,0x8C ,0xF8 ,0x24 ,0xBA ,0x30 ,0x32 ,0x50 ,0xD4 ,0xAA ,0x64 ,0x36 ,0x56 ,0xA2};
            //     memcpy(deskey, sessionKey, 24);
            //     des.init(deskey, 0);
            // } else if (authType == KEYTYPE_AES) {
            //     client.Recieve(sessionKey, 16);
            //     byte iv[16] = {0};
            //     aes.setIV(iv);
            //     aes.setKey(sessionKey, 128);
            // }
            Serial.println("Auth succesful");
            return true;
        } else {
            Serial.println(F("Wrong answer!"));
            dumpInfo(response, responseLength);
        }
    }
    return false;
}

boolean Desfire::SelectApplication(uint32_t appId) {
    Serial.println("Select Application");
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.append(DesfireCommand_SELECT_APPLICATION);
    message.append24(appId);

    byte response[32] = {0};
    byte responseLength = 32;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Select App failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (response[0] != DesfireStatusCode_OPERATION_OK) {
        Serial.println("Select App failed");
        dumpInfo(response,responseLength);
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
    message.append(DesfireCommand_DELETE_APPLICATION);
    message.append24(appId);

    byte response[32] = {0};
    byte responseLength = 32;
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
    if (appId == applicationNr) {
        applicationNr = 0;
    }
    Serial.println("App deleted");
    return true;
}

boolean Desfire::GetKeySettings(KeySettings* keySettings) {
    Serial.println("Get key settings");
    MFRC522::StatusCode status;
    Buffer<5> message;
    message.append(DesfireCommand_GET_KEY_SETTINGS);
    byte response[32] = {0};
    byte responseLength = 32;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Fetching key settings failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (response[0] != DesfireStatusCode_OPERATION_OK) {
        Serial.println("Fetching key settings failed");
        dumpInfo(response, responseLength);
        return false;
    }
    dumpInfo(response,responseLength);
    keySettings->secSettings = response[1];
    keySettings->keyCount = response[2] & 0x0F;
    keySettings->keyType = (KeyType)(response[2] & 0xF0);
    Serial.println("Key settings fetched");
    return true;
}

boolean Desfire::FormatCard() {
    Serial.println("Format card");
    MFRC522::StatusCode status;
    byte message[32] = {0};
    byte messageLength = 32;
    message[0] = DesfireCommand_FORMAT_CARD;
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

boolean Desfire::CreateApplication(uint32_t appId, byte keyCount, KeyType keyType) {
    Serial.println("Create Application");
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.append(DesfireCommand_CREATE_APPLICATION);
    message.append24(appId);
    message.append(0x0F);  // Who can change key, 0F is factory default. TODO enum;
    message.append(keyType | keyCount);
    // dumpInfo(message, 6);

    byte response[32] = {0};
    byte responseLength = 32;
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

boolean Desfire::ChangeKey(byte key[], KeyType keyType, int keyNr) {
    if (!authenticated) {
        Serial.println("Not authenticated");
        return false;
    }
    int keyLength = getKeyLength(keyType);
    boolean isSameKey = (keyNr == authkeyNr);

    byte keyVersion = 1;
    Buffer<40> message;

    message.append(DesfireCommand_CHANGE_KEY);
    if (applicationNr == 0) {
        message.append(keyNr | keyType);
    } else {
        message.append(keyNr);
    }
    byte cryptogramStart = message.size;
    message.appendBuffer(key, keyLength);
    if (!isSameKey) {
        byte oldKey[24] = {0};
        for (int i = 0; i < keyLength; i++) {
            message.buffer[i + cryptogramStart] ^= oldKey[i];
        }
    }
    if (keyType == KEYTYPE_AES) {
        message.append(keyVersion);
    }

    // Calculate CRC32
    uint32_t crc32 = ~CRC32::calculate(message.buffer, message.size);
    message.append32(crc32);
    if (!isSameKey) {
        crc32 = ~CRC32::calculate(key, keyLength);
        message.append32(crc32);
    }
    int blockSize = getBlockSize(authType);
    int lastBlockSize = (message.size - cryptogramStart) % blockSize;
    if (lastBlockSize != 0) {
        // set size to next multiple of block size
        message.pad(blockSize - lastBlockSize);
    }
    // Encrypt
    byte encDataframe[message.size] = {0};

    byte cryptogramSize = message.size - cryptogramStart;
    if (!EncryptDataframe(&message.buffer[cryptogramStart], encDataframe, cryptogramSize)) {
        Serial.println("Encryption failed");
        return false;
    }
    message.replace(cryptogramStart, encDataframe, cryptogramSize);

    MFRC522::StatusCode status;
    byte response[40] = {0};
    byte responseLength = 40;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Key change failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if (isSameKey) {
        authenticated = false;
    }
    if (response[0] != DesfireStatusCode_OPERATION_OK) {
        dumpInfo(response, responseLength);
        Serial.println("Key change failed");
        return false;
    }
    Serial.println("Key changed");
    return true;
}

uint32_t Desfire::GetAppIdFromNetwork() {
    NetworkClient client(ip);
    byte serverCommand = 0x6A;
    client.Send(&serverCommand, 1);
    client.Send(mfrc522->uid.uidByte, 7);
    byte message[3] = {0};
    client.Recieve(message, 3);
    uint32_t appId = parseAppId(message);
    return appId;
}

boolean Desfire::ChangeKeyNetwork(KeyType keyType) {
    Serial.println("Change key network");
    NetworkClient client(ip);
    MFRC522::StatusCode status;

    byte serverCommand = 0xC4;
    String name = "Big A";

    Buffer<300> message;
    message.append(serverCommand);
    message.append(keyType);
    message.appendBuffer(mfrc522->uid.uidByte, 7);
    message.append24(applicationNr);
    message.append(name.length());
    message.appendBuffer((byte*)name.c_str(), name.length());

    client.Send(message.buffer, message.size);

    byte messageLength;
    client.Recieve(&messageLength, 1);
    // TODO Check if messageLength > bufferLength
    client.Recieve(message.buffer, messageLength);

    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, messageLength, message.buffer, &messageLength);
    message.size = messageLength;
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Key change failed");
        byte code = 0xAE;
        client.Send(&code, 1);
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    authenticated = false;

    client.Send(message.buffer, 1);

    if (message.buffer[0] != DesfireStatusCode_OPERATION_OK) {
        dumpInfo(message.buffer, message.size);
        Serial.println("Key change failed");
        return false;
    }
    Serial.println("Change key succesful");
    return true;
}

int Desfire::GetAppIds(uint32_t appIds[], int maxLength) {
    MFRC522::StatusCode status;
    byte message[128] = {0};
    byte messageLength = 128;
    int ids = 0;
    message[0] = DesfireCommand_GET_APPLICATIONS;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message, 1, message, &messageLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Retrieving App Ids failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return -1;
    }
    if (message[0] != DesfireStatusCode_OPERATION_OK && message[0] != DesfireStatusCode_ADDITIONAL_FRAME) {
        Serial.println("Retrieving App Ids failed");
        dumpInfo(message, messageLength);
        return -1;
    }
    for (int i = 1; i <= messageLength - 3; i += 3) {
        appIds[ids++] = parseAppId(&message[i]);
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
            return -1;
        }
        if (message[0] != DesfireStatusCode_OPERATION_OK && message[0] != DesfireStatusCode_ADDITIONAL_FRAME) {
            Serial.println("Retrieving App Ids failed");
            dumpInfo(message, messageLength);
            return -1;
        }
        for (int i = 1; i <= messageLength - 3; i += 3) {
            appIds[ids++] = parseAppId(&message[i]);
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
    if (length % getBlockSize(authType) != 0) {
        Serial.println("Block size error");
        return false;
    }
    if (authType == KEYTYPE_3DES || authType == KEYTYPE_2K3DES) {
        des.set_size(length);
        des.tdesCbcEncipher(dataframe, encDataframe);
        Serial.println("Encrpyted");
        return true;
    } else if (authType == KEYTYPE_AES) {
        aes.encryptCBC(length, dataframe, encDataframe);
        return true;
    }
    return false;
}

int getKeyLength(KeyType keyType) {
    switch (keyType) {
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

int getBlockSize(KeyType keyType) {
    switch (keyType) {
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

int getAuthBlockSize(KeyType keyType) {
    switch (keyType) {
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
