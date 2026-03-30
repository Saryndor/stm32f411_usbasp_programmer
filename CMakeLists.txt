cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(CMAKE_MESSAGE_LOG_LEVEL VERBOSE)

set(CMAKE_TOOLCHAIN_FILE cmake/toolchain-arm-none-eabi-gcc.cmake)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)


project(firmware VERSION 0.1.0 LANGUAGES CXX C ASM)

include(cmake/stm32f411ceu6.cmake)


# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${libopencm3_SOURCE_DIR}/include
    ${libopencm3_SOURCE_DIR}/libopencm3/include/libopencm3/stm32
    ${libopencm3_SOURCE_DIR}/libopencm3/include/libopencm3/stm32/common
    ${libopencm3_SOURCE_DIR}/libopencm3/include/libopencm3/stm32/f4
)

file(GLOB SOURCES "src/*.c")


# Include the TinyUSB build instructions
include(${CMAKE_SOURCE_DIR}/cmake/tinyusb.cmake)

add_subdirectory(lib)

add_executable(${PROJECT_NAME} 
    ${SOURCES}
)

# Get the path to objcopy from the toolchain
set(OBJCOPY ${CMAKE_OBJCOPY})

# Add a post-build step to generate the .bin file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${OBJCOPY} -O binary 
            ${CMAKE_CURRENT_BINARY_DIR}/firmware.elf 
            ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin
    COMMENT "Generating binary file: firmware.bin"
    VERBATIM
)

target_link_libraries(${PROJECT_NAME} 
    usbasp
    avr_devices
    avr_spi
    hw_spi
    sw_spi
    delay
    tinyusb
    stm32f411
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
stm32_add_linker_script(${PROJECT_NAME} PRIVATE src/basic.stm32.ld)
stm32_add_flash_targets(${PROJECT_NAME})