SET "ESP_TOOLCHAIN_DIR=C:/tools/ESP"
SET "ESP_PROJECT=%cd%"

SET "CXX=xtensa-esp32-elf-g++"
SET "IOT_SOLUTION_PATH=%ESP_TOOLCHAIN_DIR%/esp-iot-solution"
SET "BOOST_ROOT=%ESP_TOOLCHAIN_DIR%/boost_1_79_0"
SET "IDF_PATH=%ESP_TOOLCHAIN_DIR%/v5.4/esp-idf"

cd /D %IDF_PATH%
call export.bat

cd /D %ESP_PROJECT%

code .

