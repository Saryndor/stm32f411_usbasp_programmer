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

# Set the path to uf2conv.py
set(UF2CONV_PY "${CMAKE_SOURCE_DIR}/uf2/uf2conv.py")
set(BOOTLOADER_BIN "${CMAKE_SOURCE_DIR}/uf2/tinyuf2.bin")
set(BOOTLOADER_PADDED_BIN  "${CMAKE_CURRENT_BINARY_DIR}/bootloader_padded.bin")

set(FIRMWARE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}" )
set(FIRMWARE_BIN  "${FIRMWARE}.bin")
set(FIRMWARE_ELF  "${FIRMWARE}.elf")
set(FIRMWARE_UF2  "${FIRMWARE}.uf2")

set(FIRMWARE_ALL "${CMAKE_CURRENT_BINARY_DIR}/bootloader_${PROJECT_NAME}" )
set(ALL_BIN  "${FIRMWARE_ALL}.bin")
set(ALL_ELF  "${FIRMWARE_ALL}.elf")

# Add a post-build step to generate the .bin and .uf2 file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD

    COMMAND ${OBJCOPY} -O binary 
            ${FIRMWARE_ELF}
            ${FIRMWARE_BIN}

    # 2. Pad Bootloader to exactly 64KB (0x10000)
    # This ensures that appending the app bin later starts exactly at 0x08010000
    COMMAND arm-none-eabi-objcopy -I binary -O binary 
                --pad-to 0x10000 --gap-fill 0xff 
                ${BOOTLOADER_BIN} ${BOOTLOADER_PADDED_BIN}

    COMMAND bash -c "cat ${BOOTLOADER_PADDED_BIN} ${FIRMWARE_BIN} > ${ALL_BIN}"


    COMMAND arm-none-eabi-ld -b binary 
                -e 0x08000000
                -Ttext 0x08000000 
                -o ${ALL_ELF} 
                ${ALL_BIN}

    COMMAND python3 ${UF2CONV_PY} -c 
            -b 0x08010000 
            -f 0x57755a57 
            -o ${FIRMWARE_UF2} 
            ${FIRMWARE_BIN} 

    COMMAND ${CMAKE_COMMAND} -E echo  "Generated files:"   
    COMMAND sh -c "ls ${CMAKE_CURRENT_BINARY_DIR} | grep -P '${PROJECT_NAME}\.(bin|elf|uf2)' | sed -E 's#(.*)#\t\\1#'"

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
# stm32_add_flash_targets(${PROJECT_NAME}  ${CMAKE_SOURCE_DIR}/uf2/tinyuf2.bin)
stm32_add_flash_targets(${ALL_ELF})
# stm32_create_combined_images(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/uf2/tinyuf2.bin)