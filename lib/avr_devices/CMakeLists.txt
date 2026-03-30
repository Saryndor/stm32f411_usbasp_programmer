set(LIB_NAME "avr_devices")
add_library(${LIB_NAME}
    avr_devices_isr.c
)

target_include_directories(${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(${CMAKE_SOURCE_DIR}/cmake/common_lib_settings.cmake)
