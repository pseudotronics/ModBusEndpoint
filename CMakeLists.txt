cmake_minimum_required(VERSION 3.12)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(ModbusEndpoint C CXX ASM)
include(PicoSDKConfig.cmake)
pico_sdk_init()



set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_executable(ModbusEndpoint
        ModbusEndpoint.cpp
        cli.c
        commands.c
        bsp_functions.c
        mb.c
        crc.c)

add_custom_command(
	TARGET ModbusEndpoint
	PRE_BUILD
	COMMAND ${CMAKE_COMMAND}
		-DHEADER_DIR:PATH="${CMAKE_SOURCE_DIR}"
		-DCACHE_DIR:PATH="${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}"
		-DLANGUAGE:STRING="C"
		-P "${CMAKE_SOURCE_DIR}/build_number.cmake"
)

target_include_directories(ModbusEndpoint PRIVATE ../../Users/Flan/AppData/Local/VisualGDB/PicoSDK/1.4.0-Package/src/rp2_common/hardware_dma/include .)
# pull in common dependencies
target_link_libraries(ModbusEndpoint pico_stdlib)

pico_enable_stdio_usb(ModbusEndpoint 1)
pico_enable_stdio_uart(ModbusEndpoint 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(ModbusEndpoint)
set_source_files_properties(circular_buffer.hpp Source.c PROPERTIES HEADER_FILE_ONLY TRUE)

# add url via pico_set_program_url
# example_auto_set_url(blink)

