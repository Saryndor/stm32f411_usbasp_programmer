# Use the local submodule directory
set(libopencm3_SOURCE_DIR "${CMAKE_SOURCE_DIR}/libopencm3")

# Check if the submodule is initialized and the Makefile exists
if(NOT EXISTS "${libopencm3_SOURCE_DIR}/Makefile")
    message(FATAL_ERROR "libopencm3 submodule not found. Please run: git submodule update --init --recursive")
endif()

# Define the library file path
set(LIBOPENCM3_LIB_FILE "${libopencm3_SOURCE_DIR}/lib/libopencm3_stm32f4.a")

# 1. Create a custom command that actually produces the library file.
# This tells Ninja/CMake how to generate the missing .a file.
add_custom_command(
    OUTPUT "${LIBOPENCM3_LIB_FILE}"
    COMMAND make TARGETS=stm32/f4 PREFIX=arm-none-eabi-
    WORKING_DIRECTORY "${libopencm3_SOURCE_DIR}"
    COMMENT "Building libopencm3 static library for STM32F4..."
    VERBATIM
)

# 2. Create a wrapper target for the custom command
add_custom_target(libopencm3_build DEPENDS "${LIBOPENCM3_LIB_FILE}")

# 3. Define the imported library target
add_library(stm32f411 STATIC IMPORTED)

# Configure include paths and the location of the compiled static library
set_target_properties(stm32f411 PROPERTIES
    IMPORTED_LOCATION "${LIBOPENCM3_LIB_FILE}"
)

target_include_directories(stm32f411 INTERFACE 
    "${libopencm3_SOURCE_DIR}/include"
    "${libopencm3_SOURCE_DIR}/lib"
)

# Ensure libopencm3 is built before the project tries to link against it
add_dependencies(stm32f411 libopencm3_build)

target_compile_definitions(stm32f411 INTERFACE -DSTM32F4 -DDEVICE=stm32f411ceu6)

# Compiler options for Cortex-M4 with Hardware FPU
set(COMPILE_OPTIONS 
  --static
  -nostartfiles
  -fno-common
  -mcpu=cortex-m4
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv4-sp-d16
)

target_compile_options(stm32f411 INTERFACE ${COMPILE_OPTIONS})
target_link_options(stm32f411 INTERFACE ${COMPILE_OPTIONS})

# Helper function for Linker Scripts
function(stm32_add_linker_script TARGET VISIBILITY SCRIPT)
    get_filename_component(SCRIPT "${SCRIPT}" ABSOLUTE)
    target_link_options(${TARGET} ${VISIBILITY} -T "${SCRIPT}")
endfunction()

# ------------------------------------------------------------------------------
# Global target to clean libopencm3
# ------------------------------------------------------------------------------
if(NOT TARGET libopencm3_clean)
    add_custom_target(libopencm3_clean
        COMMAND make clean
        WORKING_DIRECTORY "${libopencm3_SOURCE_DIR}"
        COMMENT "Cleaning libopencm3 build artifacts..."
        VERBATIM
    )
endif()

# Function to add flash targets 
function(stm32_add_flash_targets TARGET)

    add_custom_target(stlink-flash
      openocd -f /usr/share/openocd/scripts/interface/stlink.cfg
              -f /usr/share/openocd/scripts/target/stm32f4x.cfg
              -c "init"
              -c "reset halt"
              -c "reset_config none"
              -c "flash protect 0 0 3 off"
              -c "program ${TARGET} verify reset exit"
              -d0
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS ${TARGET} ${BOOTLOADER_BIN}
      VERBATIM
    )

    add_custom_target(jlink-flash
      openocd -f ${CMAKE_SOURCE_DIR}/.openocd/jlink.cfg
              -c "reset_config none"
              -c "init"
              -c "reset halt"
              -c "flash protect 0 0 3 off"
              -c "program ${TARGET} verify reset exit"
              -d0
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS ${TARGET} ${BOOTLOADER_BIN}
      VERBATIM
    )

    add_custom_target(bmp-flash
      arm-none-eabi-gdb
              -nx
              --batch
              -ex "target extended-remote host.containers.internal:2100"
              -ex "monitor swd_scan"
              -ex "mon freq 4000000"
              -ex "att 1"
              -ex "monitor halt"
              -ex "monitor unlock"
              -ex "file ${TARGET}"
              -ex "load"
              -ex "compare-sections"
              -ex "kill"
              -ex "quit"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS ${TARGET} ${BOOTLOADER_BIN}
      VERBATIM
    )

    add_custom_target(uf2-bootloader
        COMMAND stty -F /dev/ttyACM0 1200 || true
        COMMENT "Triggering 1200 baud touch to enter bootloader..."
    )
endfunction()
