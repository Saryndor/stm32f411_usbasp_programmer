set(LIB_NAME "usbasp")
add_library(${LIB_NAME}
    usbasp_dispatch.c
    usbasp_handle.c
)

target_link_libraries(${LIB_NAME}
    hw_spi
    sw_spi
    avr_spi
    delay
)

target_include_directories(${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/lib/tinyusb/src
    ${libopencm3_SOURCE_DIR}/include
    ${libopencm3_SOURCE_DIR}/libopencm3/include/libopencm3/stm32
    ${libopencm3_SOURCE_DIR}/libopencm3/include/libopencm3/stm32/common
    ${libopencm3_SOURCE_DIR}/libopencm3/include/libopencm3/stm32/f4
)

# Add required processor macros for libopencm3
target_compile_definitions(${LIB_NAME} PUBLIC 
    STM32F4 
    DEVICE=stm32f411ceu6
)
