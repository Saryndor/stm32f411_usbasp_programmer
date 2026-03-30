set(LIB_NAME "delay")
add_library(${LIB_NAME}
    delay.c
)

target_include_directories(${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(${CMAKE_SOURCE_DIR}/cmake/common_lib_settings.cmake)
