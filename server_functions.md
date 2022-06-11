# Authenticate
- -> AA KeyType(1) UID(7) AppId(3)
- -> EkRndB(8 | 16)
- <- EkRndARndBPrime(16 | 32)
- -> EkRndAPrime(8 | 16)
- TODO response:
- wenn erfolgreich:
    - <- 00
- sonst:
    - <- AE

# GetAppId
- -> 6A UID(7)
- <- AppId(3)

# ChangeKey
- -> 0xC4 KeyType(1) UID(7) AppId(3) NameLength(1) Name(NameLength)
- <- ChangeCmdLength(1) ChangeCmd(ChangeCmdLength)
- -> CmdResult(1)