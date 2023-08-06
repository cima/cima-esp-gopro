
file(GLOB_RECURSE COMPONENT_SOURCES *.cpp)

# Temporary excluding provisioning client as new middleware approach rwquires quite a lot of refactoring
list(REMOVE_ITEM COMPONENT_SOURCES "DeviceProvisioningClient.cpp")

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_DIR}/include
    REQUIRES cima-system mbedtls esp_eth lwip coreMQTT azure-sdk-for-c azure-iot-middleware-freertos azure-sample-iot)