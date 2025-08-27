# Hyperion Testing Suite CMakeLists.txt
cmake_minimum_required(VERSION 3.12)

# Test configuration
set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/test-results)
set(BENCHMARK_OUTPUT_DIR ${CMAKE_BINARY_DIR}/benchmark-results)

# Create output directories
file(MAKE_DIRECTORY ${TEST_OUTPUT_DIR})
file(MAKE_DIRECTORY ${BENCHMARK_OUTPUT_DIR})

# Test dependencies
find_package(Threads REQUIRED)

# Memory validation suite
add_executable(memory_validation_suite 
    memory_validation_suite.c
    ../core/memory.c
    ../core/config.c
    ../core/enhanced_errors.c
    ../models/text/generate.c
    ../models/text/tokenizer.c
)

# Rules engine test
add_executable(test_rules_engine
    test_rules_engine.c
    ../core/rules_engine.c
    ../core/memory.c
    ../core/config.c
    ../core/enhanced_errors.c
    ../core/quest_completion_validator.c
    ../core/simd_optimizer.c
    ../core/cross_platform_validator.c
)

# Gemini integration test
add_executable(test_gemini
    test_gemini.c
    ../core/config.c
    ../interface/gemini_cli.c
)

target_include_directories(memory_validation_suite PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/interface
)

target_link_libraries(memory_validation_suite 
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Rules engine test configuration
target_include_directories(test_rules_engine PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/interface
)

target_link_libraries(test_rules_engine 
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Gemini test configuration
target_include_directories(test_gemini PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/interface
)

target_link_libraries(test_gemini 
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if(WIN32)
    target_link_libraries(memory_validation_suite psapi)
endif()

# Performance benchmark suite
add_executable(performance_benchmark_suite 
    performance_benchmark_suite.c
    ../core/memory.c
    ../core/config.c
    ../core/enhanced_errors.c
    ../models/text/generate.c
    ../models/text/tokenizer.c
)

target_include_directories(performance_benchmark_suite PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/interface
)

target_link_libraries(performance_benchmark_suite 
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if(WIN32)
    target_link_libraries(performance_benchmark_suite psapi)
endif()

# Compiler-specific flags for tests
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(memory_validation_suite PRIVATE 
        -Wall -Wextra -Wno-unused-parameter -std=c99
    )
    target_compile_options(performance_benchmark_suite PRIVATE 
        -Wall -Wextra -Wno-unused-parameter -std=c99
    )
    target_compile_options(test_rules_engine PRIVATE 
        -Wall -Wextra -Wno-unused-parameter -std=c99
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(memory_validation_suite PRIVATE -g -O0)
        target_compile_options(performance_benchmark_suite PRIVATE -g -O0)
        target_compile_options(test_rules_engine PRIVATE -g -O0)
    endif()
endif()

# Enable testing
enable_testing()

# Add CTest tests
add_test(NAME MemoryValidation 
         COMMAND memory_validation_suite
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

add_test(NAME PerformanceBenchmark 
         COMMAND performance_benchmark_suite
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

add_test(NAME RulesEngine 
         COMMAND test_rules_engine
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

add_test(NAME GeminiIntegration
         COMMAND test_gemini
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

# Set test properties
set_tests_properties(MemoryValidation PROPERTIES
    TIMEOUT 300
    LABELS "memory;validation;core"
)

set_tests_properties(PerformanceBenchmark PROPERTIES
    TIMEOUT 600
    LABELS "performance;benchmark;optimization"
)

set_tests_properties(RulesEngine PROPERTIES
    TIMEOUT 300
    LABELS "validation;rules;core"
)

set_tests_properties(GeminiIntegration PROPERTIES
    TIMEOUT 300
    LABELS "integration;gemini;cli"
)

# Custom test targets
add_custom_target(test-memory
    COMMAND ${CMAKE_CTEST_COMMAND} -L memory --output-on-failure
    DEPENDS memory_validation_suite
    COMMENT "Running memory validation tests"
)

add_custom_target(test-performance
    COMMAND ${CMAKE_CTEST_COMMAND} -L performance --output-on-failure
    DEPENDS performance_benchmark_suite
    COMMENT "Running performance benchmark tests"
)

add_custom_target(test-rules
    COMMAND ${CMAKE_CTEST_COMMAND} -L rules --output-on-failure
    DEPENDS test_rules_engine
    COMMENT "Running rules engine tests"
)

add_custom_target(test-gemini
    COMMAND ${CMAKE_CTEST_COMMAND} -L gemini --output-on-failure
    DEPENDS test_gemini
    COMMENT "Running Gemini integration tests"
)

add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS memory_validation_suite performance_benchmark_suite test_rules_engine test_gemini
    COMMENT "Running all tests"
)

# Memory debugging options
if(ENABLE_MEMORY_DEBUGGING)
    target_compile_definitions(memory_validation_suite PRIVATE 
        HYPERION_MEMORY_DEBUG=1
        HYPERION_TRACK_ALLOCATIONS=1
    )
endif()

# Valgrind integration (Linux only)
if(UNIX AND NOT APPLE)
    find_program(VALGRIND_PROGRAM valgrind)
    if(VALGRIND_PROGRAM)
        add_custom_target(test-valgrind
            COMMAND ${VALGRIND_PROGRAM} 
                    --tool=memcheck
                    --leak-check=full
                    --show-leak-kinds=all
                    --track-origins=yes
                    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/valgrind.supp
                    --error-exitcode=1
                    $<TARGET_FILE:memory_validation_suite>
            DEPENDS memory_validation_suite
            COMMENT "Running memory validation with Valgrind"
        )
    endif()
endif()

# Code coverage (GCC/Clang with debug build)
if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(memory_validation_suite PRIVATE --coverage)
        target_link_options(memory_validation_suite PRIVATE --coverage)
        target_compile_options(performance_benchmark_suite PRIVATE --coverage)
        target_link_options(performance_benchmark_suite PRIVATE --coverage)
        
        add_custom_target(coverage
            COMMAND gcov ${CMAKE_CURRENT_BINARY_DIR}/*.gcno
            COMMAND lcov --capture --directory ${CMAKE_CURRENT_BINARY_DIR} --output-file coverage.info
            COMMAND genhtml coverage.info --output-directory coverage-html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report"
        )
    endif()
endif()

# Test result formatting
add_custom_target(test-report
    COMMAND ${CMAKE_COMMAND} -E echo "=== Hyperion Test Results ==="
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS memory_validation_suite performance_benchmark_suite
    COMMENT "Generating comprehensive test report"
)

# Install test executables (optional)
if(INSTALL_TESTS)
    install(TARGETS memory_validation_suite performance_benchmark_suite
            RUNTIME DESTINATION bin/tests
            COMPONENT testing)
    
    install(FILES valgrind.supp
            DESTINATION share/hyperion/tests
            COMPONENT testing)
endif()