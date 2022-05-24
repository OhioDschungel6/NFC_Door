cmake_minimum_required(VERSION 3.14)

set(ARDUINO_BOARD esp32.esp32)

project(NFC C CXX)

add_executable(NFC src/NetworkClient.cpp src/NetworkManager.cpp src/NFC_Reader.cpp)
include_directories(/home/andi/.arduino15/packages/esp32/hardware/esp32/2.0.2/libraries/WiFi/src)
target_link_arduino_libraries(NFC PRIVATE core MFRC522  WiFi WiFiManager)
#MFRC522v2
#target_link_arduino_libraries(NFC AUTO_PUBLIC)
target_enable_arduino_upload(NFC)