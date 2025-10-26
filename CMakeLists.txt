cmake_minimum_required(VERSION 3.10)

# Hyperion Project
project(Hyperion VERSION 0.1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler flags
if(MSVC)
    add_compile_options(/W4)
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
set(HYPERION_UTILS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/quantize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/simd_ops.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/performance_monitor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/deployment_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/monitoring_center.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/auto_scaler.c
)
set(HYPERION_MODELS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/models/model_format.c
    ${CMAKE_CURRENT_SOURCE_DIR}/models/text/sampling.c
)

# Exclude legacy duplicates and unfinished experimental modules until reconciled
list(FILTER HYPERION_CORE_SOURCES EXCLUDE REGEX ".*/picol_fixes\\.c$")
list(FILTER HYPERION_CORE_SOURCES EXCLUDE REGEX ".*/rules_engine\\.c$")
list(FILTER HYPERION_CORE_SOURCES EXCLUDE REGEX ".*/memory_optimization_validator\\.c$")
list(FILTER HYPERION_CORE_SOURCES EXCLUDE REGEX ".*/platform_compatibility_validator\\.c$")
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

option(HYPERION_BUILD_MAIN "Build Hyperion main executable" OFF)
option(HYPERION_BUILD_ROS2 "Build Hyperion ROS 2 integration" OFF)

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

add_library(hyperion_ros2 STATIC
    ros2/text_to_action_node.c
    ros2/perception_bridge.c
    ros2/control_bridge.c
    ros2/autonomy_pipeline.c
    ros2/autonomy_monitor.c
)

target_include_directories(hyperion_ros2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(hyperion_ros2 PRIVATE
    hyperion_models
    hyperion_utils
)

if(UNIX)
    target_link_libraries(hyperion_ros2 PRIVATE m)
endif()

# Link math library to libraries if needed (Unix/Linux)
if(UNIX)
    target_link_libraries(hyperion_core PRIVATE m)
    target_link_libraries(hyperion_models PRIVATE m)
    target_link_libraries(hyperion_utils PRIVATE m)
endif()

# Main executable (optional)
if(HYPERION_BUILD_MAIN)
    add_executable(hyperion
        main.c # Add main.c back explicitly
        ${HYPERION_MAIN_INTERFACE_SOURCES} # Use the explicit list
    )

    target_link_libraries(hyperion PRIVATE
        hyperion_core
        hyperion_models
        hyperion_utils
    )

    if(UNIX)
        target_link_libraries(hyperion PRIVATE m)
    endif()

    if(WIN32)
        target_link_libraries(hyperion PRIVATE ws2_32)
    endif()

    install(TARGETS hyperion
        RUNTIME DESTINATION bin
    )

    install(DIRECTORY data/
        DESTINATION share/hyperion/data
        PATTERN "*.md" EXCLUDE
    )

    install(FILES
        README.md
        ARCHITECTURE.md
        IMPLEMENTATION_PLAN.md
        DESTINATION share/doc/hyperion
    )
endif()

# Enable testing
enable_testing()

set(HYPERION_TEST_SOURCES
    tests/test_framework.c
    tests/test_memory.c
    tests/test_model_format.c
    tests/test_sampling.c
    tests/test_io.c
    tests/test_ros2_stub.c
    tests/test_ros2_pipeline_stub.c
    tests/test_ros2_monitor_stub.c
    tests/test_performance_monitor.c
    tests/test_deployment_manager.c
    tests/test_monitoring_center.c
    tests/test_auto_scaler.c
)

# Test executable
add_executable(hyperion_tests
    tests/test_main.c # Explicitly list main test file
    ${HYPERION_TEST_SOURCES} # Add other test files
)

# Link math library and internal libs for tests
if(UNIX)
    target_link_libraries(hyperion_tests PRIVATE m)
endif()

target_link_libraries(hyperion_tests PRIVATE
    hyperion_core
    hyperion_models
    hyperion_utils
    hyperion_ros2
)

# Add aggregate test target
add_test(NAME HyperionTests COMMAND hyperion_tests)

option(HYPERION_BUILD_EXAMPLES "Build Hyperion examples" OFF)

if(HYPERION_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(HYPERION_BUILD_ROS2)
    find_package(rcl QUIET)
    find_package(rclc QUIET)
    find_package(rclc_executor QUIET)
    find_package(std_msgs QUIET)
    find_package(geometry_msgs QUIET)

    if(NOT rcl_FOUND OR NOT rclc_FOUND OR NOT rclc_executor_FOUND OR NOT std_msgs_FOUND OR NOT geometry_msgs_FOUND)
        message(FATAL_ERROR "HYPERION_BUILD_ROS2 is ON but required ROS 2 packages were not found.")
    endif()

    target_compile_definitions(hyperion_ros2 PUBLIC HYPERION_HAVE_ROS2)

    target_link_libraries(hyperion_ros2 PRIVATE
        hyperion_core
        hyperion_models
        hyperion_utils
        rcl::rcl
        rclc::rclc
        rclc_executor::rclc_executor
        std_msgs::std_msgs
        geometry_msgs::geometry_msgs
    )
endif()

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