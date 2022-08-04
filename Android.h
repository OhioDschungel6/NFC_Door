#pragma once
#include <MFRC522Extended.h>

#include "NetworkClient.h"
#include "Utils.h"

class Android {
   public:
    Android(MFRC522Extended* mfrc522, String ip);
    boolean SelectApplication(Buffer<7> aid);
    boolean Verify();
    boolean GetKey();

   private:
    MFRC522Extended* mfrc522;
    String ip;
    byte currentUid[16];
};

enum AndroidCommand : byte {
    AndroidCommand_SELECT_APPLICATION = 0xA4,   
};
