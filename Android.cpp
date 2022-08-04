#include "Android.h"

Android::Android(MFRC522Extended* mfrc522,String ip) {
    this->mfrc522 = mfrc522;
    this->ip = ip;
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
    dumpInfo(response,responseLength);
    memcpy(currentUid, response, 16);
    Serial.println("App selected");
    return true;
}

boolean Android::Verify(){
    Serial.println("Verify Android");
    NetworkClient client(ip);
    byte cmd = 0x4A;
    client.Send(&cmd,1);
    client.Send(currentUid,16);
    byte rndNmr[16];
    client.Recieve(rndNmr,16);
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.append(cmd);
    message.appendBuffer(rndNmr,16);
    byte response[100] = {0};
    byte responseLength = 100;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Verify failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        dumpInfo(response,responseLength);
        return false;
    }
    client.Send(&responseLength,1);
    client.Send(response,responseLength);
    dumpInfo(response,responseLength);
    Serial.println("Verify end");
    return true;
}

boolean Android::GetKey(){
    Serial.println("GetKey Android");
    MFRC522::StatusCode status;
    byte cmd = 0x56;
    Buffer<32> message;
    message.append(cmd);
    byte response[128] = {0};
    byte responseLength = 128;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, message.buffer, message.size, response, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Get Key failed");
        Serial.println(MFRC522::GetStatusCodeName(status));
        dumpInfo(response,responseLength);
        return false;
    }
    NetworkClient client(ip);
    client.Send(&cmd,1);
    client.Send(currentUid,16);
    client.Send(&responseLength,1);
    client.Send(response,responseLength);
    dumpInfo(response,responseLength);
    Serial.println("Get Key end");
    return true;
}
