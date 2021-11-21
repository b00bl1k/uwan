add_executable(test_sx127x
    test_sx127x.c
    ../src/device/sx127x.c)
add_test(NAME test_sx127x COMMAND test_sx127x)

add_executable(test_stack
    test_stack.c
    ../src/stack.c
    ${TINYCRYPT_SRC})
add_test(NAME test_stack COMMAND test_stack)
