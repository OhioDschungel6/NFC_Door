esptool.exe --chip esp32 --port COM4 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0xe000 boot_app0.bin 0x1000 NFC_Door.ino.bootloader.bin 0x10000 NFC_Door.ino.bin 0x8000 NFC_Door.ino.partitions.bin
esptool.exe --chip esp32 --port COM4 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 2686976 NFC_Door.spiffs.bin
