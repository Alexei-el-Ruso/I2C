cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "/home/alexei-el-ruso/pico/pico-sdk")

include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(i2c C CXX)

pico_sdk_init()

add_executable(i2c main.c)

target_link_libraries(
        i2c
        pico_stdlib
        hardware_i2c
)

pico_add_extra_outputs(i2c)

pico_enable_stdio_usb(i2c 1)
pico_enable_stdio_uart(i2c 0)