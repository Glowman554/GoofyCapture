cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)
include(pico-sdk/tools/CMakeLists.txt)

project(pico)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
set(TARGET pico)

add_executable(${TARGET}
    main.c
    readline.c
)

include_directories(${TARGET} 
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

# pull in common dependencies
target_link_libraries(${TARGET} 
    pico_stdlib
    hardware_pio
    hardware_dma
)

# enable usb output & uart output
pico_enable_stdio_usb(${TARGET} 1)
pico_enable_stdio_uart(${TARGET} 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${TARGET})
