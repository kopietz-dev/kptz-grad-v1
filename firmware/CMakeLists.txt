cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(rp2040 C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(program
    main.cpp
    usb_descriptors.c
)

target_include_directories(program PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(program
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_spi
)

pico_enable_stdio_usb(program 1)
pico_enable_stdio_uart(program 0)

pico_add_extra_outputs(program)

set(PICO_MOUNT_PATH /media/$ENV{USER}/RPI-RP2)


add_custom_target(flash
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:program>/program.uf2
            /run/media/$ENV{USER}/RPI-RP2/program.uf2
    COMMENT "Flashing UF2 manually"
)
