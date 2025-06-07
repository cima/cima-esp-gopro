
file(GLOB_RECURSE COMPONENT_SOURCES *.cpp)

# Boost
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_DIR}/include
    REQUIRES esp_driver_gpio esp_driver_ledc mbedtls esp_eth lwip esp_wifi)