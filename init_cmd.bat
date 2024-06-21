SET "ESP_TOOLCHAIN_DIR=C:/tools/ESP"
SET "ESP_PROJECT=%cd%"

SET "CXX=xtensa-esp32-elf-g++"
SET "IOT_SOLUTION_PATH=%ESP_TOOLCHAIN_DIR%/esp-iot-solution"
SET "BOOST_ROOT=%ESP_TOOLCHAIN_DIR%/boost_1_79_0"
SET "IDF_PATH=%ESP_TOOLCHAIN_DIR%/esp-idf"

cd /D %ESP_TOOLCHAIN_DIR%/esp-framework
call idf_cmd_init.bat

cd /D %ESP_PROJECT%



