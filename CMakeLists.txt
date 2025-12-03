cmake_minimum_required(VERSION 3.10)
project(Hyperion VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(MSVC)
    add_compile_options(/W4) # Use MSVC warning level 4
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB HYPERION_CORE_SOURCES "core/*.c")
file(GLOB HYPERION_INTERFACE_SOURCES "interface/*.c")

add_executable(hyperion
    main.c
    ${HYPERION_CORE_SOURCES}
    ${HYPERION_INTERFACE_SOURCES}
)

if(UNIX)
    target_link_libraries(hyperion PRIVATE m)
endif()

enable_testing()
file(GLOB HYPERION_TEST_SOURCES "tests/*.c")
add_executable(hyperion_tests
    ${HYPERION_TEST_SOURCES}
    ${HYPERION_CORE_SOURCES}
    ${HYPERION_INTERFACE_SOURCES}
)

if(UNIX)
    target_link_libraries(hyperion_tests PRIVATE m)
endif()

add_test(NAME CliTests COMMAND hyperion_tests)

add_custom_target(scaffold_core_module
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/core/scaffold_core_module.py --name sample_module --description "Prototype scaffold"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generate a prototype core module (adjust arguments as needed)"
)
