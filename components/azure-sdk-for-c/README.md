# azure-sdk-for-c

This seemingly empty component being built is a way how Microsoft 
is letting you to customize their azure-iot-middleware for FreeRTOS.

By building this set of libraries implementing calls to Azure services
you make them available for usage further in azure-iot-middleware for FreeRTOS.

This particular CMakeLists.txt file is taken from [Microsofts example](https://github.com/Azure-Samples/iot-middleware-freertos-samples/tree/main/demos/projects/ESPRESSIF/esp32)

> Moreover this is a way how overall ESP-IDF built system allows you to
> organize your code by separating parts of your code to independent components.
> Components that can even exists in separate folder and repository.