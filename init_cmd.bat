SET "ESP_TOOLCHAIN_DIR=C:/tools/ESP"
SET "ESP_PROJECT=%cd%"

SET "CXX=xtensa-esp32-elf-g++"
SET "IOT_SOLUTION_PATH=%ESP_TOOLCHAIN_DIR%/esp-iot-solution"
SET "BOOST_ROOT=%ESP_TOOLCHAIN_DIR%/boost_1_79_0"
SET "IDF_PATH=%ESP_TOOLCHAIN_DIR%/master/esp-idf"
rem J:\src\cima\esp-idf-mirf
SET "NRF24_COMPONENTS_PATH=../esp-idf-mirf"
SET "CIMA_COMPONENTS_PATH=../cima-esp32-components"

cd /D %IDF_PATH%
call export.bat

cd /D %ESP_PROJECT%

code .

