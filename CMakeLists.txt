cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)

project(thermometer)

pico_sdk_init()
add_executable(thermometer thermometer.c)

target_link_libraries(thermometer pico_stdlib)
