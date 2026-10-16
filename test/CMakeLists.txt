######################################################
# Apache License, Version 2.0
# Copyright (c) 2026 chciken/Niko
######################################################

add_custom_target(tests)

set(TEST_INCLUDE_PATHS ${CMAKE_SOURCE_DIR}/vcml/include
                       ${CMAKE_BINARY_DIR}/vcml/gen/
                       src/
                       src/models/)
set(TEST_LIB_PATHS -L${SYSTEMC_PATH}/lib-linux64)
set(TEST_LIBS arduvp_test vcml -lgcov -lgtest)
set(TEST_COMPILE_OPTS --coverage -g -O0)

macro(create_test_case test_name)
  add_executable(${test_name} ${test_name}.cpp testing.cpp)
  add_dependencies(tests ${test_name} vcml arduvp_test)
  target_include_directories(${test_name} PUBLIC ${TEST_INCLUDE_PATHS})
  target_link_libraries(${test_name} ${TEST_LIB_PATHS})
  target_compile_options(${test_name} PUBLIC ${TEST_COMPILE_OPTS})
  target_link_libraries(${test_name} ${TEST_LIBS})
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 300)
endmacro()

create_test_case(adc)
create_test_case(bmp180)
create_test_case(comparator)
create_test_case(cpu)
create_test_case(eeprom_24xx)
create_test_case(io_ports)
create_test_case(system)
create_test_case(timer)
create_test_case(twi)
create_test_case(usart)
create_test_case(ssd1306)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(SW_WORKLOADS
  adc
  bmp180
  builtin_led
  serial/hello_world
  mat_mul
  timer
)

foreach(workload ${SW_WORKLOADS})
  add_test(NAME sw_${workload}
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/sw_test.py
                   --binary $<TARGET_FILE:arduvp> ${workload})
  set_tests_properties(sw_${workload} PROPERTIES TIMEOUT 90)
endforeach()

add_dependencies(tests arduvp)
