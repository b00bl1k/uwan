cmake_minimum_required(VERSION 3.5)

project(uwan C)

set(CMAKE_C_STANDARD 99)

set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${INC_DIR})

set(TINYCRYPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinycrypt)
include_directories(${TINYCRYPT_DIR}/lib/include)
set(TINYCRYPT_SRC
    ${TINYCRYPT_DIR}/lib/source/aes_encrypt.c
    ${TINYCRYPT_DIR}/lib/source/cmac_mode.c
    ${TINYCRYPT_DIR}/lib/source/utils.c)

set(LIB_SRC
    ${SRC_DIR}/device/radio.c
    ${SRC_DIR}/device/sx127x.c
    ${SRC_DIR}/stack.c)

add_library(uwan STATIC ${LIB_SRC} ${TINYCRYPT_SRC})
set_target_properties(uwan PROPERTIES OUTPUT_NAME uwan)

enable_testing()

add_subdirectory(tests)
