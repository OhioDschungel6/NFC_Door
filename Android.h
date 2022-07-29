#pragma once
#include <MFRC522Extended.h>

#include "NetworkClient.h"
#include "Utils.h"

class Android {
   public:
    Android(MFRC522Extended* mfrc522);
    boolean SelectApplication(Buffer<7> aid);
    boolean Verify();

   private:
    MFRC522Extended* mfrc522;
};

enum AndroidCommand : byte {
    AndroidCommand_SELECT_APPLICATION = 0xA4,   
};
