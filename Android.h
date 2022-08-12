#pragma once
#include <MFRC522Extended.h>

#include "NetworkClient.h"
#include "Utils.h"

class Android {
   public:
    Android(MFRC522Extended* mfrc522, String ip);
    boolean SelectApplication(byte aid [7]);
    boolean Verify();
    boolean GetKey(String name, const unsigned char presharedKey[16]);
    boolean CheckIsAvailable();

   private:
    MFRC522Extended* mfrc522;
    String ip;
    byte currentUid[16];
};

enum AndroidCommand : byte {
    AndroidCommand_SELECT_APPLICATION = 0xA4,   
};
