set(LIB_NAME "avr_spi")
add_library(${LIB_NAME}
    avr_spi.c
)

target_link_libraries(${LIB_NAME}
    avr_devices
    hw_spi
    sw_spi
    delay
)

include(${CMAKE_SOURCE_DIR}/cmake/common_lib_settings.cmake)