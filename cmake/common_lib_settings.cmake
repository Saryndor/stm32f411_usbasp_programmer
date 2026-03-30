# Add common include directories
target_include_directories(${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
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

target_compile_options(${LIB_NAME} PUBLIC
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)