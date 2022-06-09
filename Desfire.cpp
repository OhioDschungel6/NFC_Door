#include "Desfire.h" 
Desfire::Desfire(MFRC522Extended* mfrc522)
{
    this->mfrc522 = mfrc522;
}

boolean Desfire::AuthenticateNetwork(int keytype, int keyNr)
{
    byte cmd;
    switch (keytype) {
        case KEYTYPE_3DES: cmd = 0x1A;
            break;
        case KEYTYPE_AES: cmd = 0xAA;
            break;
        default:
            //Not implemented
            return false;
    }
    NetworkClient client;
    byte AuthBuffer[64] = {0}; //
    byte AuthLength = 64;
    byte message[64] = {0}; // Message to transfer

    byte authMode[1] = {0};
    byte blockSize;
    if(keytype == KEYTYPE_3DES){
        authMode[0] = 0;
        blockSize=8;
    }else if(keytype == KEYTYPE_AES){
        authMode[0] = 1;
        blockSize = 16;
    }
    //Notify server if 3DES or AES
    client.Send(authMode,1);

    //#Step 0: Get and send id
    client.Send(mfrc522->uid.uidByte, 7);

    //Start Authentification
    MFRC522::StatusCode status;
    //Step - 1
    // Build command buffer
    AuthBuffer[0] = cmd; 
    AuthBuffer[1] = keyNr; 

    // Transmit the buffer and receive the response
    //Step - 2
    status = mfrc522->TCL_Transceive(&mfrc522->tag,AuthBuffer,2,AuthBuffer,&AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Desfire Auth failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    }
    if(AuthBuffer[0] != DesfireStatusCode_ADDITIONAL_FRAME){
        Serial.println("Desfire Auth failed");
        //TODO ERROR CODE
        return false;
    }
    
    memcpy(message, AuthBuffer + 1, blockSize); // copy the enc(RndB) from the message
    
    //Step - 3
    client.Send(message, blockSize); //ek(RndB)
    client.Recieve(message, blockSize*2); // ek(RndA || RndB')
    AuthBuffer[0] = DesfireStatusCode_ADDITIONAL_FRAME;
    memcpy(AuthBuffer + 1, message, blockSize*2); 
    //Step - 4
    status =  mfrc522->TCL_Transceive(&mfrc522->tag,AuthBuffer,1+ 2* blockSize,AuthBuffer,&AuthLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Auth failed failed: "));
        Serial.println(MFRC522::GetStatusCodeName(status));
        return false;
    } else {
        if (AuthBuffer[0] == DesfireStatusCode_OPERATION_OK) {                        // reply from PICC should start with 0x00
            memcpy(message, &AuthBuffer[1], 16);               // copy enc(RndA')
            client.Send(message, 16); //ek(RndA')
            dumpInfo(message, 16);
            authenticated = true;
            Serial.println("Auth succesful");
            return true;
        } else {
            Serial.println(F("Wrong answer!"));
            dumpInfo(AuthBuffer, AuthLength);
        }
    }
    return false;
}

boolean  Desfire::ChangeKey(byte key [], int keytype)
{
    if(keytype != KEYTYPE_AES){
        Serial.println("Changing key to any other than aes not supported.")
        return false;
    }
    if(!authenticated){
        Serial.println("Not authenticated")
        return false;
    }
    int keyLength = getKeyLength(keytype);
    if(applicationNr == 0){
        //Masterkey
        int keyVersion = 0;
        byte message[32] = {0};
        byte messageLength = 32;

        message[0] = 0xC4; 
        message[1] = keytype; 
        memcpy(message+2, key, 16);
        message[19] = keyVersion;

        //Calculate CRC32

        //Encrypt
        encryptDataframe();

        status = mfrc522->TCL_Transceive(&mfrc522->tag,message,32,message,&messageLength);
        if (status != MFRC522::STATUS_OK) {
            Serial.println("Desfire Auth failed");
            Serial.println(MFRC522::GetStatusCodeName(status));
            return false;
        }
        if(AuthBuffer[0] != DesfireStatusCode_ADDITIONAL_FRAME){
            Serial.println("Desfire Auth failed");
            //TODO ERROR CODE
            return false;
        }
    } else {
        //TODO other key
        Serial.println("Currently not supported")
        return false;
    }
    
}

boolean encryptDataframe(byte dataframe[], int length){

}

int getKeyLength(int keytype){
    switch (keytype)
    {
    case KEYTYPE_3DES:
        return 24;
    case KEYTYPE_AES:
        return 16;
    default:
        return 0;
    }
}
