cmake_minimum_required(VERSION 3.5)

project(uwan C)

set(CMAKE_C_STANDARD 99)

set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(TINYCRYPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinycrypt)
set(TINYCRYPT_INC ${TINYCRYPT_DIR}/lib/include)
set(TINYCRYPT_SRC
    ${TINYCRYPT_DIR}/lib/source/aes_encrypt.c
    ${TINYCRYPT_DIR}/lib/source/cmac_mode.c
    ${TINYCRYPT_DIR}/lib/source/utils.c)

set(LIB_SRC
    ${SRC_DIR}/device/sx127x.c
    ${SRC_DIR}/device/sx126x.c
    ${SRC_DIR}/ext/clock_sync.c
    ${SRC_DIR}/region/common.c
    ${SRC_DIR}/region/eu868.c
    ${SRC_DIR}/region/ru864.c
    ${SRC_DIR}/adr.c
    ${SRC_DIR}/channels.c
    ${SRC_DIR}/mac.c
    ${SRC_DIR}/stack.c
    ${SRC_DIR}/utils.c)

add_library(uwan STATIC ${LIB_SRC} ${TINYCRYPT_SRC})
target_include_directories(uwan
    PRIVATE
        ${SRC_DIR}
        ${TINYCRYPT_INC}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${INC_DIR}>
)
set_target_properties(uwan PROPERTIES OUTPUT_NAME uwan)

option(BUILD_TESTING "Build and run tests" ON)
if (${BUILD_TESTING})
    enable_testing()

    set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
    include(CodeCoverage)
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE ctest
        EXCLUDE "tests/*" "vendor/*"
    )

    append_coverage_compiler_flags()

    add_subdirectory(tests)
endif()
