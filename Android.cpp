#include "Android.h"

Android::Android(MFRC522Extended* mfrc522,IPAddress ip,unsigned int port) {
    this->mfrc522 = mfrc522;
    this->ip = ip;
    this->port = port;
}

boolean Android::SelectApplication(byte aid[7]){
    Serial.println("Select Application Android");
    MFRC522::StatusCode status;
    Buffer<32> message;
    message.append(0x00);
    message.append(AndroidCommand_SELECT_APPLICATION);
    message.append(0x04);
    message.append(0x00);
    message.append(0x07);
    message.appendBuffer(aid,7);
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
    NetworkClient client(ip,port);
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

boolean Android::GetKey(String name, const unsigned char presharedKey[16]){
    if(name.length() >= 50){
      return false;
    }
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
    
    NetworkClient client(ip,port);
    client.Send(&cmd,1);

    Buffer<512> sendBuffer;
    sendBuffer.appendBuffer(currentUid, 16);
    
    byte nameLength = (byte) name.length();
    sendBuffer.append(nameLength);
    sendBuffer.appendBuffer((byte*)name.c_str(),(byte)name.length());
    sendBuffer.append(responseLength);
    sendBuffer.appendBuffer(response,responseLength);
    client.SendWithHMAC(sendBuffer.buffer, sendBuffer.size,presharedKey);
    
    Serial.println("Get Key end");
    return true;
}

boolean Android::CheckIsAvailable(){
    byte cmd = 0xCA;
    byte responseLength;
    MFRC522::StatusCode status;
    status = mfrc522->TCL_Transceive(&mfrc522->tag, &cmd, 1, &cmd, &responseLength);
    if (status != MFRC522::STATUS_OK) {
        return false;
    }
    if(cmd == 0x1A){
      return true;
    }else{
      return false;
    }
    
}

boolean Android::IsAndroidKnown() {
    NetworkClient client(ip, port);
    byte serverCommand = 0xA6;
    client.Send(&serverCommand, 1);
    client.Send(currentUid, 16);
    byte message;
    client.Recieve(&message, 1);
    return message == 1;
}
