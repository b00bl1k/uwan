set(TESTS
    sx127x
    stack)

foreach(test ${TESTS})
    add_executable(test_${test} test_${test}.c)
    target_link_libraries(test_${test} uwan)
    add_test(NAME test_${test} COMMAND $<TARGET_FILE:test_${test}>)
endforeach()
