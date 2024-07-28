set(TESTS
    sx127x
    sx126x
    )

foreach(test ${TESTS})
    add_executable(test_${test} test_${test}.c)
    target_include_directories(test_${test} PRIVATE ${SRC_DIR})
    target_link_libraries(test_${test} uwan)
    add_test(NAME test_${test} COMMAND $<TARGET_FILE:test_${test}>)
endforeach()

add_executable(test_mac
    test_mac.c
    ${SRC_DIR}/mac.c
    ${SRC_DIR}/channels.c
    ${SRC_DIR}/utils.c
)
target_include_directories(test_mac PRIVATE
    ${SRC_DIR}
    ${INC_DIR}
)
add_test(NAME test_mac COMMAND test_mac)

add_executable(test_channels
    test_channels.c
    ${SRC_DIR}/region/common.c
    ${SRC_DIR}/region/eu868.c
    ${SRC_DIR}/adr.c
    ${SRC_DIR}/channels.c
    ${SRC_DIR}/mac.c
    ${SRC_DIR}/stack.c
    ${TINYCRYPT_SRC}
)
target_include_directories(test_channels PRIVATE
    ${SRC_DIR}
    ${INC_DIR}
    ${TINYCRYPT_INC}
)
add_test(NAME test_channels COMMAND test_channels)

add_executable(test_stack
    test_stack.c
    ${SRC_DIR}/region/common.c
    ${SRC_DIR}/region/eu868.c
    ${SRC_DIR}/adr.c
    ${SRC_DIR}/channels.c
    ${SRC_DIR}/mac.c
    ${SRC_DIR}/stack.c
    ${TINYCRYPT_SRC}
)
target_include_directories(test_stack PRIVATE
    ${SRC_DIR}
    ${INC_DIR}
    ${TINYCRYPT_INC}
)
add_test(NAME test_stack COMMAND test_stack)
