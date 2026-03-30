# ==============================================================================
# TinyUSB integration for STM32F411 (Synopsys DWC2)
# ==============================================================================

# Define the path relative to THIS file (cmake/tinyusb.cmake)
set(TINYUSB_DIR "${CMAKE_CURRENT_LIST_DIR}/../lib/tinyusb")

# Define the source files
set(TINYUSB_SRC
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/class/vendor/vendor_device.c
    ${TINYUSB_DIR}/src/portable/synopsys/dwc2/dcd_dwc2.c
    ${TINYUSB_DIR}/src/class/cdc/cdc_device.c
    ${TINYUSB_DIR}/src/portable/synopsys/dwc2/dwc2_common.c
)

# Create the static library
add_library(tinyusb STATIC ${TINYUSB_SRC})

# ------------------------------------------------------------------------------
# Include Directories
# ------------------------------------------------------------------------------

# PUBLIC includes: Accessible to your main application (e.g., to include "tusb.h")
target_include_directories(tinyusb PUBLIC
    ${TINYUSB_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# PRIVATE includes: CMSIS headers required by dcd_synopsys.c
# Kept private to avoid naming conflicts with libopencm3 in your main app!
target_include_directories(tinyusb PRIVATE
    ${TINYUSB_DIR}/hw/mcu/st/cmsis_device_f4/Include
    ${TINYUSB_DIR}/lib/CMSIS_5/CMSIS/Core/Include
)

# ------------------------------------------------------------------------------
# Compiler Definitions
# ------------------------------------------------------------------------------

# Tell the TinyUSB stack which architecture it is being compiled for
target_compile_definitions(tinyusb PUBLIC
    CFG_TUSB_MCU=OPT_MCU_STM32F4
)

# Tell the ST CMSIS headers exactly which chip is used
target_compile_definitions(tinyusb PRIVATE
    STM32F411xE
)

# ==============================================================================
# Compiler Options (Architecture Flags)
# ==============================================================================
target_compile_options(tinyusb PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)