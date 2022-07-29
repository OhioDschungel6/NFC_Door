#include "Android.h"

Android::Android(MFRC522Extended* mfrc522) {
    this->mfrc522 = mfrc522;
}

boolean Android::SelectApplication(Buffer<7> aid){
    Serial.println("Select Application Android");
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.append(0x00);
    message.append(AndroidCommand_SELECT_APPLICATION);
    message.append(0x04);
    message.append(0x00);
    message.append(0x07);
    message.appendBuffer(aid.buffer,aid.size);
    message.append(0x00);
    

    dumpInfo(message.buffer,message.size);
    byte response[32] = {0};
    byte responseLength = 32;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Select App failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        dumpInfo(response,responseLength);
        return false;
    }
    Serial.println("App selected");
    return true;
}

boolean Android::Verify(){
    Serial.println("Verify Android");
    byte randomNumber[16]={0x00}; //getfromserver
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.appendBuffer(randomNumber,16);
    byte response[64] = {0};
    byte responseLength = 64;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Verify failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        dumpInfo(response,responseLength);
        return false;
    }
    dumpInfo(response,responseLength);
    Serial.println("Verified");
    return true;
}
