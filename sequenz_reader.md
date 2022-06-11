# Seqeuenz Reader (Hardware)
- Event: Karte aufgelegt:
    - -> Server: ist UID bekannt
    - <- Server: App ID oder 0
    - wenn ID 0:
        - return
    - -> Server: Authenticate AES, UID, AppId
    - wenn erfolgreich:
        - Tür öffnen