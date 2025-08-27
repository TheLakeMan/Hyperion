cmake_minimum_required(VERSION 3.10)

# Hyperion Project
project(Hyperion VERSION 0.1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler flags
if(MSVC)
    add_compile_options(/W4 /FS)  # /FS flag fixes PDB file conflicts
    # Enable SIMD instructions for MSVC
    add_compile_options(/arch:SSE2)
    option(ENABLE_AVX "Enable AVX instructions" ON)
    if(ENABLE_AVX)
        add_compile_options(/arch:AVX)
    endif()
    option(ENABLE_AVX2 "Enable AVX2 instructions" ON)
    if(ENABLE_AVX2)
        add_compile_options(/arch:AVX2)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Enable SIMD instructions for GCC/Clang
    add_compile_options(-msse2)
    option(ENABLE_AVX "Enable AVX instructions" ON)
    if(ENABLE_AVX)
        add_compile_options(-mavx)
    endif()
    option(ENABLE_AVX2 "Enable AVX2 instructions" ON)
    if(ENABLE_AVX2)
        add_compile_options(-mavx2)
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor # Add vendor directory
)

# Collect source files
file(GLOB_RECURSE HYPERION_CORE_SOURCES "core/*.c")
file(GLOB_RECURSE HYPERION_UTILS_SOURCES "utils/*.c")
file(GLOB_RECURSE HYPERION_MODELS_SOURCES "models/*.c")

# Rules Engine sources (part of core)
set(HYPERION_RULES_ENGINE_SOURCES
    core/rules_engine.c
    core/memory_optimization_validator.c
    core/platform_compatibility_validator.c
    core/simd_optimizer.c
    core/cross_platform_validator.c
)

# Explicitly list interface sources for the main executable (excluding interface/main.c)
set(HYPERION_MAIN_INTERFACE_SOURCES
    interface/cli.c
    interface/web_server.c  # Web server interface
    # Add other interface files here if needed for hyperion.exe
)

# Define MCP_ENABLED to enable Model Context Protocol support
add_compile_definitions(MCP_ENABLED)

# Define the HYPERION_BUILD preprocessor symbol to exclude main() from picol.c
add_compile_definitions(HYPERION_BUILD)

# Define HYPERION_RULES_ENGINE_ENABLED for GODMODE enhancement features
add_compile_definitions(HYPERION_RULES_ENGINE_ENABLED)

# Options for enhanced validation
option(ENABLE_MEMORY_VALIDATION "Enable enhanced memory validation" ON)
option(ENABLE_PLATFORM_VALIDATION "Enable platform compatibility validation" ON)
option(ENABLE_QUEST_VALIDATION "Enable quest completion validation" ON)
option(ENABLE_SIMD_VALIDATION "Enable SIMD optimization validation" ON)

if(ENABLE_MEMORY_VALIDATION)
    add_compile_definitions(HYPERION_MEMORY_VALIDATION_ENABLED)
endif()

if(ENABLE_PLATFORM_VALIDATION)
    add_compile_definitions(HYPERION_PLATFORM_VALIDATION_ENABLED)
endif()

if(ENABLE_QUEST_VALIDATION)
    add_compile_definitions(HYPERION_QUEST_VALIDATION_ENABLED)
endif()

if(ENABLE_SIMD_VALIDATION)
    add_compile_definitions(HYPERION_SIMD_VALIDATION_ENABLED)
endif()

# Create library targets for use by examples and tests
add_library(hyperion_core STATIC
    ${HYPERION_CORE_SOURCES}
)

add_library(hyperion_models STATIC
    ${HYPERION_MODELS_SOURCES}
)

add_library(hyperion_utils STATIC
    ${HYPERION_UTILS_SOURCES}
)

# Link math library to libraries if needed (Unix/Linux)
if(UNIX)
    target_link_libraries(hyperion_core PRIVATE m)
    target_link_libraries(hyperion_models PRIVATE m)
    target_link_libraries(hyperion_utils PRIVATE m)
endif()

# Main executable
add_executable(hyperion
    main.c # Add main.c back explicitly
    ${HYPERION_MAIN_INTERFACE_SOURCES} # Use the explicit list
)

# Link the libraries to the main executable
target_link_libraries(hyperion PRIVATE
    hyperion_core
    hyperion_models
    hyperion_utils
)

# Check if math library is needed (Unix/Linux)
if(UNIX)
    target_link_libraries(hyperion PRIVATE m)
endif()

# Link network library if needed (e.g., for Mongoose)
if(WIN32)
    target_link_libraries(hyperion PRIVATE ws2_32)
endif()

# Create install rules
install(TARGETS hyperion
    RUNTIME DESTINATION bin
)

# Install data directories
install(DIRECTORY data/
    DESTINATION share/hyperion/data
    PATTERN "*.md" EXCLUDE
)

# Install documentation
install(FILES
    README.md
    ARCHITECTURE.md
    IMPLEMENTATION_PLAN.md
    DESTINATION share/doc/hyperion
)

# Enable testing
enable_testing()

# Collect test source files (excluding main)
file(GLOB HYPERION_TEST_SOURCES "tests/test_*.c")

# Add our new memory optimization validator test
list(APPEND HYPERION_TEST_SOURCES "tests/test_memory_optimization_validator.c")

# Add our new configuration test
list(APPEND HYPERION_TEST_SOURCES "tests/test_config.c")

# Add our new integration tests
list(APPEND HYPERION_TEST_SOURCES "tests/test_integration.c")
list(APPEND HYPERION_TEST_SOURCES "tests/test_mcp_integration.c")
list(APPEND HYPERION_TEST_SOURCES "tests/test_cross_platform.c")
list(APPEND HYPERION_TEST_SOURCES "tests/test_performance.c")
list(APPEND HYPERION_TEST_SOURCES "tests/test_stress.c")
list(APPEND HYPERION_TEST_SOURCES "tests/test_concurrency.c")
list(APPEND HYPERION_TEST_SOURCES "tests/test_rules_engine.c")
list(APPEND HYPERION_TEST_SOURCES "tests/test_phase_verification.c")

# Test executable
add_executable(hyperion_tests
    tests/test_main.c # Explicitly list main test file
    ${HYPERION_TEST_SOURCES} # Add other test files
    ${HYPERION_CORE_SOURCES} # Now including picol.c with HYPERION_BUILD defined
    ${HYPERION_UTILS_SOURCES}
    ${HYPERION_MODELS_SOURCES}
    # Add interface/cli.c explicitly as it's needed by tests but not in other lists
    interface/cli.c 
)

# Link math library for tests if needed
if(UNIX)
    target_link_libraries(hyperion_tests m)
endif()

# Add tests
add_test(NAME CoreTests COMMAND hyperion_tests core)
add_test(NAME UtilsTests COMMAND hyperion_tests utils)
add_test(NAME ModelsTests COMMAND hyperion_tests models)
add_test(NAME SimdTests COMMAND hyperion_tests simd)
add_test(NAME SparseMatrixTests COMMAND hyperion_tests sparse)
add_test(NAME MultimodalTests COMMAND hyperion_tests multimodal)

# Sparse matrix operations test executable
add_executable(sparse_matrix_test
    tests/test_sparse_ops.c
    ${HYPERION_UTILS_SOURCES}
)

# Link math library for sparse matrix test if needed
if(UNIX)
    target_link_libraries(sparse_matrix_test m)
endif()

# Image model test executable
add_executable(image_test
    models/image/image_test.c
    models/image/image_model.c
    models/image/image_utils.c
    ${HYPERION_UTILS_SOURCES}
)

# Link math library for image test if needed
if(UNIX)
    target_link_libraries(image_test m)
endif()

# Audio model test executable
add_executable(audio_test
    tests/test_audio_model.c
    models/audio/audio_model.c
    # TODO: Implement these audio module files:
    # models/audio/audio_features.c
    # models/audio/audio_utils.c
    ${HYPERION_UTILS_SOURCES}
)

# Link math library for audio test if needed
if(UNIX)
    target_link_libraries(audio_test m)
endif()

# Attention mechanism test executable
add_executable(attention_test
    tests/test_attention.c
    models/text/attention.c
    models/text/attention.h
    ${HYPERION_UTILS_SOURCES}
)

# Link math library for attention test if needed
if(UNIX)
    target_link_libraries(attention_test m)
endif()

# SIMD benchmark test executable
add_executable(simd_benchmark_test
    tests/test_simd_benchmark.c
    utils/simd_benchmark.c
    models/text/attention.c
    ${HYPERION_UTILS_SOURCES}
)

# Link math library for SIMD benchmark test if needed
if(UNIX)
    target_link_libraries(simd_benchmark_test m)
endif()

# Add a specific test for SIMD benchmarks
add_test(NAME SimdBenchmarkTest COMMAND simd_benchmark_test)

# Cache optimization test executable
add_executable(cache_opt_test
    tests/test_cache_opt.c
    utils/cache_opt.c
    ${HYPERION_UTILS_SOURCES}
)

# Link math library for cache optimization test if needed
if(UNIX)
    target_link_libraries(cache_opt_test m)
endif()

# Add a specific test for cache optimizations
add_test(NAME CacheOptimizationTest COMMAND cache_opt_test)

# Multimodal test executable
add_executable(multimodal_test
    tests/test_multimodal.c
    models/multimodal/multimodal_model.c
    models/multimodal/fusion.c
    ${HYPERION_UTILS_SOURCES}
    ${HYPERION_CORE_SOURCES}
    models/image/image_model.c
    models/image/image_utils.c
)

# Link math library for multimodal test if needed
if(UNIX)
    target_link_libraries(multimodal_test m)
endif()

# Add a specific test for multimodal capabilities
add_test(NAME MultimodalTest COMMAND multimodal_test)

# Memory-mapped model loader and forward scheduler test executable
add_executable(mmap_loader_test
    tests/test_mmap_loader.c
    utils/mmap_loader.c
    utils/forward_scheduler.c
    ${HYPERION_UTILS_SOURCES}
)

# Link math library for memory-mapped model loader test if needed
if(UNIX)
    target_link_libraries(mmap_loader_test m)
endif()

# Add a specific test for memory-mapped model loading and forward scheduling
add_test(NAME MemoryMappedModelTest COMMAND mmap_loader_test)

# Out-of-memory handling test executable
add_executable(oom_handling_test
    tests/test_oom_handling.c
    utils/memory_pool.c
    utils/mmap_loader.c
    utils/forward_scheduler.c
    ${HYPERION_UTILS_SOURCES}
)

# Link math library for OOM test if needed
if(UNIX)
    target_link_libraries(oom_handling_test m)
endif()

# Add a specific test for out-of-memory handling
add_test(NAME OutOfMemoryHandlingTest COMMAND oom_handling_test

# Rules Engine Verification executable
add_executable(verify_rules_engine
    verify_rules_engine.c
    ${HYPERION_CORE_SOURCES}
    ${HYPERION_UTILS_SOURCES}
    ${HYPERION_MODELS_SOURCES}
)

# Link math library for rules engine verification if needed
if(UNIX)
    target_link_libraries(verify_rules_engine m)
endif())

# Add examples
add_subdirectory(examples)

# Create a package
set(CPACK_PACKAGE_NAME "hyperion")
set(CPACK_PACKAGE_VENDOR "Hyperion Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ultra-Lightweight AI Framework")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Hyperion")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Set package generators
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
else()
    set(CPACK_GENERATOR "ZIP;TGZ;DEB;RPM")
endif()

include(CPack)