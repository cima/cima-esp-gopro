cmake_minimum_required(VERSION 3.20)

set(AZURE_SDK_C_PATH $ENV{AZURE_SDK_C_PATH})

set(CMAKE_SYSTEM_NAME Generic)

# ESP32 imports
# ------------- 
message(INFO " IoT solution is at: $ENV{IOT_SOLUTION_PATH}")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_C_FLAGS -mlongcalls ${CMAKE_C_FLAGS})
set(CMAKE_CXX_FLAGS -mlongcalls ${CMAKE_CXX_FLAGS})

cmake_policy(SET CMP0074 NEW)
find_package(Boost)

# Adafruit sensor
# ---------------
list(APPEND IOT_COMPONENT_DIRS
                        $ENV{IOT_SOLUTION_PATH}/components/bus)

list(APPEND EXTRA_COMPONENT_DIRS ${IOT_COMPONENT_DIRS})

# Main code
# ---------

project(CIMA-ESP32-GOPRO)
