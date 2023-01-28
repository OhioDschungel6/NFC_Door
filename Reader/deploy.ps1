./esptool.exe --chip esp32 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0xe000 boot_app0.bin 0x1000 NFC_Door.ino.bootloader.bin 0x10000 NFC_Door.ino.bin 0x8000 NFC_Door.ino.partitions.bin
./mkspiffs.exe -c data -p 256 -b 4096 -s 1507328 spiffs.bin
./esptool.exe --chip esp32 --port COM4 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 2686976 spiffs.bin
