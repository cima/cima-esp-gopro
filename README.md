# Cima-ESP32-LightHub

# Development

Based on Microsoft's Azure IoT sample [Connect an ESPRESSIF ESP32 using Azure IoT Middleware for FreeRTOS](https://github.com/Azure-Samples/iot-middleware-freertos-samples/tree/main/demos/projects/ESPRESSIF/esp32)

## Prerequisities

- Python 3.9+
- [Git](https://git-scm.com/download/win)
- [CP210x USB to UART Bridge VCP Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
  - Download v.11.1.0. Unzip. Rightclick _silabser.inf_ and select **install**. Follow instrucitons.
  - Plug-in your Daplink-enabled ESP-32 to USB. Serial port (Silicon Labs CP210x USB to UART Bridge
) should appear in device management.
- [Putty](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html)
  - Create serial port connection to port determined in previous step
  - Speed (baud): 115 200
  - Data bits: 8, stopbit: 1, flow control: XON/XOFF, parity: none
- [VSCode](https://code.visualstudio.com/)
  - [C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
  - [ESP32 extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)

- [BOOST]()


## Install toolchain and libraries
1. Clone this project somewhere e.g. `./cima-esp32-azure/`
2. Create separate ESP toolchain and libraries folder e.g. `./ESP/`
```
    mkdir ./ESP/
    cd ./ESP/
```
3. Clone _ESP-IDF_ to folder from step 2. e.g. `./ESP/esp-idf/`
```
    git clone -b release/v4.4 --recursive https://github.com/espressif/esp-idf.git
```
4. Clone _Azure IoT Middleware for FreeRTOS_ to folder from step 2. e.g. `./ESP/azure-iot-middleware-freertos/`
```
    git clone --recursive https://github.com/Azure/azure-iot-middleware-freertos.git
    git submodule update --init --recursive
```
5. Clone _ESP-IoT-solution_ to folder from step 2. e.g. `./ESP/esp-iot-solution/`
```
    git clone --recursive https://github.com/espressif/esp-iot-solution.git
```
6. get Boost via [Version 1.79.0](https://www.boost.org/users/history/version_1_79_0.html) and unpack it to folder from step 2. E.g. `./ESP/boost_1_79_0/`
7. ESP 32 toolchain v4.0 (incl. gcc, cmake, ninja) -- Windows
  - [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html)
    - Downlaods all the necesary tools for development (including COM drivers)

8. Update file `init_cmd.bat` in this project (from step 1) so the first variable `ESP_TOOLCHAIN_DIR` contains the absolute prefix of your toolchain directory.
9. Fisrt build
```
  idf.py build
```

Notes:
> Boot mode: Some boards might be shipped with fast boot as a default option. To use `idf.py flash` you should switch to download boot mode. See in [Manual Bootloader](https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html#manual-bootloader)