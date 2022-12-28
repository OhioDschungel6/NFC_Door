# WithHMAC
- <- Nonce(32)
- -> MsgLength(4) Msg(MsgLength) HMAC(Msg || Nonce)

# Authenticate
- -> AA KeyType(1) UID(7) AppId(3)
- -> EkRndB(8 | 16)
- <- EkRndARndBPrime(16 | 32)
- -> EkRndAPrime(8 | 16)
- wenn erfolgreich:
    - <- 00
- sonst:
    - <- AE

# Open Door
- -> 0D KeyType(1) UID(7) AppId(3)
- -> EkRndB(8 | 16)
- <- EkRndARndBPrime(16 | 32)
- -> EkRndAPrime(8 | 16)
- wenn erfolgreich:
    - <- 00
- sonst:
    - <- AE

# Get App Id
- -> 6A UID(7)
- wenn App exisitert:
    - <- AppId(3)
- sonst:
    - <- Zero(3)

# Verify Android
- -> 4A UID(16)
- <- DataToSign(16)
- -> Length(1) SignedData(Length)
- wenn erfolgreich:
    - <- 00
- sonst:
    - <- AE

# Save Public Key
- -> 56 WithHMAC(UID(16) NameLength(1) Name(NameLength) KeyLength(1), PublicKey(KeyLength))
- wenn erfolgreich:
    - <- 00
- sonst:
    - <- 5E

# Get All Devices
- -> 6D
- <- Length(4) Json({"desfire": [[uid, name], ...], "android": [[uid, name], ...]}")

# Delete Key
- -> DD WithHMAC(UID(16))
- wenn erfolgreich:
    - <- 00
- sonst:
    - <- 5E

# Is Key Known
- -> 66 UID(7)
- wenn Schlüssel bekannt:
    - <- 00
- sonst:
    - <- 01

# Is Android Device Known
- -> A6 UID(16)
- wenn Schlüssel bekannt:
    - <- 00
- sonst:
    - <- 01

# ChangeKey
- -> 0xC4 WithHMAC(KeyType(1) UID(7) AppId(3) NameLength(1) Name(NameLength))
- <- ChangeCmdLength(1) MsgLength(1) Msg(MsgLength) [Msg = Cmd(1) || KeyNr(1) || Enc(ChangeCmd)]
- -> WithHMAC(StatusCode(1))
- wenn erfolgreich:
    - <- 00
- sonst:
    - <- 5E
