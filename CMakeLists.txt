######################################################
# Apache License, Version 2.0
# Copyright (c) 2025 chciken/Niko
######################################################

cmake_minimum_required(VERSION 3.5)
project(arduvp VERSION 1.0.0 LANGUAGES C CXX)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CTest)
include(CPack)
enable_testing()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "-Wno-deprecated")
set(CMAKE_CXX_STANDARD 23)
add_definitions(-DSC_DISABLE_API_VERSION_CHECK) # "Fix" c++ version issue.
add_definitions(-DSC_INCLUDE_DYNAMIC_PROCESSES)

# Set with -DARDUVP_ASAN=ON.
option(ARDUVP_ASAN "Build with AddressSanitizer enabled" OFF)
if(ARDUVP_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()

include_directories(${CMAKE_SOURCE_DIR}/src)

set(ALL_CPPS
  ${CMAKE_SOURCE_DIR}/src/adc.cpp
  ${CMAKE_SOURCE_DIR}/src/comparator.cpp
  ${CMAKE_SOURCE_DIR}/src/cpu.cpp
  ${CMAKE_SOURCE_DIR}/src/eeprom_24xx256.cpp
  ${CMAKE_SOURCE_DIR}/src/io_ports.cpp
  ${CMAKE_SOURCE_DIR}/src/insn.cpp
  ${CMAKE_SOURCE_DIR}/src/main.cpp
  ${CMAKE_SOURCE_DIR}/src/ssd1306.cpp
  ${CMAKE_SOURCE_DIR}/src/timer.cpp
  ${CMAKE_SOURCE_DIR}/src/twi.cpp
  ${CMAKE_SOURCE_DIR}/src/usart.cpp
)

add_subdirectory(vcml)

# The main executable.
add_executable(arduvp ${ALL_CPPS})
target_include_directories(arduvp SYSTEM PUBLIC  ${CMAKE_SOURCE_DIR}/vcml/include ${CMAKE_BINARY_DIR}/vcml/gen/)
target_link_libraries(arduvp vcml)
target_compile_options(arduvp PUBLIC -g -O3)
add_dependencies(arduvp vcml)

# Debug.
add_executable(arduvp_debug ${ALL_CPPS})
target_include_directories(arduvp_debug SYSTEM PUBLIC  ${CMAKE_SOURCE_DIR}/vcml/include ${CMAKE_BINARY_DIR}/vcml/gen/)
target_link_libraries(arduvp_debug vcml)
target_compile_options(arduvp_debug PUBLIC -O0 -g)
add_dependencies(arduvp_debug vcml)

# For tests.
add_library(arduvp_test STATIC ${ALL_CPPS})
target_include_directories(arduvp_test SYSTEM PUBLIC   ${CMAKE_SOURCE_DIR}/vcml/include ${CMAKE_BINARY_DIR}/vcml/gen/)
target_link_libraries(arduvp_test vcml)
target_compile_options(arduvp_test PUBLIC -O0 -g --coverage)
add_dependencies(arduvp_test vcml)

add_subdirectory(test)
