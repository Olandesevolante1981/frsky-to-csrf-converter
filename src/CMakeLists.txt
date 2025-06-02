cmake_minimum_required(VERSION 3.13)

# Include the Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(frsky_to_crsf C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add executable
add_executable(frsky_to_crsf
    src/main.c
    src/frsky_sport.c
    src/crsf.c
    src/telemetry_converter.c
)

# Pull in our pico_stdlib which aggregates commonly used features
target_link_libraries(frsky_to_crsf 
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_timer
    hardware_irq
    hardware_flash
    hardware_sync
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(frsky_to_crsf 1)
pico_enable_stdio_uart(frsky_to_crsf 0)

# Create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(frsky_to_crsf)
