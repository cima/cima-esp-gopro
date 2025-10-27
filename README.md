# Cima-ESP32-GoPro

An ESP32 based Wi-Fi client connecting to a Go Pro camera sending commands over HTTP. 
Variety of IOpins allows to trigger commands by other technologies like 433MHz wireless protocols, 
current detectors or simply by cable with switch. 

These enable different Go Pro controll mechanisms as well as range extension.
Regular wi-fi mobile app is usable only few meters, require phone to be unlock and running,
not speking about environment and mount needs for cellphone.

# Development

## Prerequisities

- Python 3.11+
- [Git](https://git-scm.com/download/win)
- [CP210x USB to UART Bridge VCP Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
  - Download v.11.1.0. Unzip. Rightclick _silabser.inf_ and select **install**. Follow instrucitons.
  - Plug-in your Daplink-enabled ESP-32 to USB. Serial port (Silicon Labs CP210x USB to UART Bridge
) should appear in device management.
- [Putty](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html) \[optional\]
  - Create serial port connection to port determined in previous step
  - Speed (baud): 115 200
  - Data bits: 8, stopbit: 1, flow control: XON/XOFF, parity: none
- [VSCode](https://code.visualstudio.com/)
  - [C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
  - [ESP32 extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)


## Install toolchain and libraries
1. Clone this project somewhere e.g. `./cima-esp32-gopro/`
2. Follow espressif's instruction in [Get Started](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html)
3. Clone _ESP-IoT-solution_ to folder from step 2. e.g. `./ESP/esp-iot-solution/`

  ```bash
  git clone --recursive https://github.com/espressif/esp-iot-solution.git
  ```
4. get Boost via [Version 1.79.0](https://www.boost.org/users/history/version_1_79_0.html) and unpack it to some folder. E.g. `./ESP/boost_1_79_0/`

5. Clone esp-idf-mirf

  ```bash
  git clone https://github.com/nopnop2002/esp-idf-mirf.git
  ```

and update the location in `NRF24_COMPONENTS_PATH` in [init_cmd.bat](init_cmd.bat) if it is not cloned side by side of this project

5. Clone cima-esp32-components

  ```bash
  git clone https://github.com/cima/cima-esp32-components.git
  ```

  and update the location in `CIMA_COMPONENTS_PATH` in [init_cmd.bat](init_cmd.bat) if it is not cloned side by side of this project

6. Update file [init_cmd.bat](init_cmd.bat) in this project (from step 1) so the first variable `ESP_TOOLCHAIN_DIR` contains the absolute prefix of your toolchain directory.

7. Fisrt build

  ```bash
  idf.py build
  ```

### Old instructions (too manual)
1. Clone this project somewhere e.g. `./cima-esp32-gopro/`
2. Create separate ESP toolchain and libraries folder e.g. `./ESP/`
```
    mkdir ./ESP/
    cd ./ESP/
```
3. Clone _ESP-IDF_ to folder from step 2. e.g. `./ESP/esp-idf/`
```
    git clone -b release/v4.4 --recursive https://github.com/espressif/esp-idf.git
```
4. Clone _ESP-IoT-solution_ to folder from step 2. e.g. `./ESP/esp-iot-solution/`
```
    git clone --recursive https://github.com/espressif/esp-iot-solution.git
```
5. get Boost via [Version 1.79.0](https://www.boost.org/users/history/version_1_79_0.html) and unpack it to folder from step 2. E.g. `./ESP/boost_1_79_0/`
6. ESP 32 toolchain v4.0 (incl. gcc, cmake, ninja) -- Windows
  - [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html)
    - Downlaods all the necesary tools for development (including COM drivers)
7. Update file `init_cmd.bat` in this project (from step 1) so the first variable `ESP_TOOLCHAIN_DIR` contains the absolute prefix of your toolchain directory.
8. Fisrt build
```
idf.py build
```

## Compilation

1. Start this project using modified `init_cmd.bat` this ensures that VS Code hase every variable and path available and thus intellisense and code browsing works smoothly.
2. Build the project using idf.py

```bash
idf.py build
```

### WIFI problem 
Decompile static library to assembler
```bash
C:\tools\ESP\esp-framework\tools\xtensa-esp-elf\esp-14.2.0_20241119\xtensa-esp-elf\bin\xtensa-esp32s2-elf-objdump.exe -DCrz -Mintel C:\tools\ESP\v5.4\esp-idf\components\esp_wifi\lib\esp32\libnet80211.a > libnet80211.asm
```
> From [multimedia.cx's Objdump](https://wiki.multimedia.cx/index.php/Objdump)

Read partition. The offset and the size are taken from partitions.csv
```bash
python -m esptool --port COM4 read_flash 0x9000 0x6000 partition.bin
```

dump partition to human readable format
```bash
C:\tools\ESP\master\esp-idf\components\nvs_flash\nvs_partition_tool\nvs_tool.py -f text -d all partition.bin
```

## Debugging

convert Dumped Bcatrace to readable functin names and line numbers

```bat
xtensa-esp32-elf-addr2line -e J:\src\cima\cima-esp32-gopro\build\CIMA-ESP32-GOPRO.elf  0x40081b0d:0x3ffb1f40 0x400890e5:0x3ffb1f60
```

## Wiring

| nRF24L01  | ESP-WROOM-32   |
|-----------|----------------|
|  IRQ      |  GPIO17        |
|  MISO     |  GPIO19        |
|  MOSI     |  GPIO23        |
|  SCK      |  GPIO18        |
|  CSN      |  GPIO5         |
|  CE       |  GPIO16        |

# Notes:
> Boot mode: Some boards might be shipped with fast boot as a default option. To use `idf.py flash` you should switch to download boot mode. See in [Manual Bootloader](https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html#manual-bootloader)

# Reading 

- [ESP HTTP Client](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_client.html)

- [IP_EVENT_STA_GOT_IP](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#ip-event-sta-got-ip)