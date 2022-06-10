#pragma once
#include <AES32.h>
#include <DES.h>
#include <MFRC522Extended.h>

#include "NetworkClient.h"
#include "Utils.h"

class Desfire {
   public:
    Desfire(MFRC522Extended* mfrc522);
    boolean AuthenticateNetwork(int keytype, int keyNr);
    boolean ChangeKey(byte key[], int keytype, int keyNr);
    boolean SelectApplication(uint32_t appId);
    boolean CreateApplication(uint32_t appId, byte keyCount, int keyType);
    boolean DeleteApplication(uint32_t appId);
    boolean FormatCard();
    int GetAppIds(uint32_t appIds[], int maxLength);

   private:
    MFRC522Extended* mfrc522;
    boolean authenticated = false;
    uint32_t applicationNr = 0;
    byte sessionKey[24];
    int authkeyNr;
    int AuthType;
    DES des = DES();
    AES32 aes = AES32();
    boolean EncryptDataframe(byte dataframe[], byte encDataframe[], int length);
};
enum KeyTypes {
    KEYTYPE_2K3DES = 0x00,
    KEYTYPE_3DES = 0x40,
    KEYTYPE_AES = 0x80,
};
enum DesfireStatusCode : byte {
    DesfireStatusCode_OPERATION_OK = 0x00,          /* successful operation */
    DesfireStatusCode_NO_CHANGES = 0x0C,            /* no changes done to backup files */
    DesfireStatusCode_OUT_OF_EEPROM_ERROR = 0x0E,   /* insufficient NV-Mem. to complete cmd */
    DesfireStatusCode_ILLEGAL_COMMAND_CODE = 0x1C,  /* command code not supported */
    DesfireStatusCode_INTEGRITY_ERROR = 0x1E,       /* CRC or MAC does not match data */
    DesfireStatusCode_NO_SUCH_KEY = 0x40,           /* invalid key number specified */
    DesfireStatusCode_LENGTH_ERROR = 0x7E,          /* length of command string invalid */
    DesfireStatusCode_PERMISSION_ERROR = 0x9D,      /* curr conf/status doesnt allow cmd */
    DesfireStatusCode_PARAMETER_ERROR = 0x9E,       /* value of the parameter(s) invalid */
    DesfireStatusCode_APPLICATION_NOT_FOUND = 0xA0, /* requested AID not present on PICC */
    DesfireStatusCode_APPL_INTEGRITY_ERROR = 0xA1,  /* unrecoverable err within app */
    DesfireStatusCode_AUTHENTICATION_ERROR = 0xAE,  /* cur auth status doesnt allow req cmd */
    DesfireStatusCode_ADDITIONAL_FRAME = 0xAF,      /* additional data frame to be sent */
    DesfireStatusCode_BOUNDARY_ERROR = 0xBE,        /* attempt to read/write beyond limits */
    DesfireStatusCode_PICC_INTEGRITY_ERROR = 0xC1,  /* unrecoverable error within PICC */
    DesfireStatusCode_COMMAND_ABORTED = 0xCA,       /* previous command not fully completed */
    DesfireStatusCode_PICC_DISABLED_ERROR = 0xCD,   /* PICC disabled by unrecoverable error */
    DesfireStatusCode_COUNT_ERROR = 0xCE,           /* cant create more apps, already @ 28 */
    DesfireStatusCode_DUPLICATE_ERROR = 0xDE,       /* cant create dup. file/app */
    DesfireStatusCode_EEPROM_ERROR = 0xEE,          /* couldnt complete NV-write operation */
    DesfireStatusCode_FILE_NOT_FOUND = 0xF0,        /* specified file number doesnt exist */
    DesfireStatusCode_FILE_INTEGRITY_ERROR = 0xF1   /* unrecoverable error within file */
};
