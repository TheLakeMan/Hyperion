# Embedded Systems Build Configuration for Hyperion
cmake_minimum_required(VERSION 3.12)

project(hyperion_embedded)

# Embedded platform detection
if(DEFINED ARDUINO_BOARD)
    set(PLATFORM_ARDUINO ON)
    message(STATUS "Building Hyperion for Arduino platform: ${ARDUINO_BOARD}")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    set(PLATFORM_RASPBERRY_PI ON)
    message(STATUS "Building Hyperion for Raspberry Pi")
else()
    set(PLATFORM_GENERIC_EMBEDDED ON)
    message(STATUS "Building Hyperion for generic embedded platform")
endif()

# Common embedded settings
set(EMBEDDED_COMPILE_FLAGS
    "-DHYPERION_EMBEDDED=1"
    "-DHYPERION_MINIMAL_MEMORY=1"
    "-DHYPERION_NO_FILESYSTEM=1"
    "-DHYPERION_4BIT_QUANTIZATION=1"
    "-Os"  # Optimize for size
    "-ffunction-sections"
    "-fdata-sections"
    "-fno-exceptions"
    "-fno-rtti"
)

# Arduino-specific configuration
if(PLATFORM_ARDUINO)
    message(STATUS "Configuring for Arduino embedded platform")
    
    # Arduino memory constraints
    add_definitions(
        -DHYPERION_ARDUINO=1
        -DHYPERION_VOCAB_SIZE=500
        -DHYPERION_HIDDEN_SIZE=32
        -DHYPERION_NUM_LAYERS=2
        -DHYPERION_MAX_SEQUENCE=64
        -DHYPERION_MAX_TOKENS=25
        -DHYPERION_MEMORY_LIMIT_KB=32
    )
    
    # Arduino-optimized source files (minimal set)
    set(HYPERION_EMBEDDED_SOURCES
        hyperion_arduino.c
        ../../../core/memory_minimal.c
        ../../../core/config_minimal.c
        ../../../models/text/generate_minimal.c
        ../../../models/text/tokenizer_minimal.c
    )
    
    # Arduino library
    add_library(hyperion_arduino STATIC
        ${HYPERION_EMBEDDED_SOURCES}
    )
    
    target_compile_options(hyperion_arduino PRIVATE ${EMBEDDED_COMPILE_FLAGS})
    
    # Arduino-specific linker flags
    set_target_properties(hyperion_arduino PROPERTIES
        LINK_FLAGS "-Wl,--gc-sections -Wl,--print-memory-usage"
    )

# Raspberry Pi configuration
elseif(PLATFORM_RASPBERRY_PI)
    message(STATUS "Configuring for Raspberry Pi embedded platform")
    
    # Raspberry Pi optimizations
    add_definitions(
        -DHYPERION_RASPBERRY_PI=1
        -DHYPERION_VOCAB_SIZE=2000
        -DHYPERION_HIDDEN_SIZE=64
        -DHYPERION_NUM_LAYERS=4
        -DHYPERION_MAX_SEQUENCE=128
        -DHYPERION_MAX_TOKENS=100
        -DHYPERION_MEMORY_LIMIT_MB=64
    )
    
    # Check for NEON support on ARM
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        list(APPEND EMBEDDED_COMPILE_FLAGS "-mfpu=neon" "-mfloat-abi=hard")
        add_definitions(-DHYPERION_NEON=1)
        message(STATUS "Enabling NEON SIMD for Raspberry Pi")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
        add_definitions(-DHYPERION_NEON=1)
        message(STATUS "Enabling NEON SIMD for ARM64 Raspberry Pi")
    endif()
    
    # Raspberry Pi source files
    set(HYPERION_EMBEDDED_SOURCES
        hyperion_raspberry_pi.c
        ../../../core/memory.c
        ../../../core/config.c
        ../../../models/text/generate.c
        ../../../models/text/tokenizer.c
        ../../../utils/tensor.c
    )
    
    # Raspberry Pi library
    add_library(hyperion_rpi STATIC
        ${HYPERION_EMBEDDED_SOURCES}
    )
    
    target_compile_options(hyperion_rpi PRIVATE ${EMBEDDED_COMPILE_FLAGS})
    
    # Link math library
    target_link_libraries(hyperion_rpi m)

# Generic embedded platform
else()
    message(STATUS "Configuring for generic embedded platform")
    
    # Generic embedded settings
    add_definitions(
        -DHYPERION_GENERIC_EMBEDDED=1
        -DHYPERION_VOCAB_SIZE=1000
        -DHYPERION_HIDDEN_SIZE=48
        -DHYPERION_NUM_LAYERS=3
        -DHYPERION_MAX_SEQUENCE=96
        -DHYPERION_MAX_TOKENS=50
        -DHYPERION_MEMORY_LIMIT_MB=16
    )
    
    # Generic embedded source files
    set(HYPERION_EMBEDDED_SOURCES
        hyperion_generic_embedded.c
        ../../../core/memory.c
        ../../../core/config.c
        ../../../models/text/generate.c
        ../../../models/text/tokenizer.c
    )
    
    # Generic embedded library
    add_library(hyperion_embedded STATIC
        ${HYPERION_EMBEDDED_SOURCES}
    )
    
    target_compile_options(hyperion_embedded PRIVATE ${EMBEDDED_COMPILE_FLAGS})
    
    # Link math library if available
    find_library(MATH_LIB m)
    if(MATH_LIB)
        target_link_libraries(hyperion_embedded ${MATH_LIB})
    endif()
endif()

# Common include directories for all embedded platforms
foreach(target IN ITEMS hyperion_arduino hyperion_rpi hyperion_embedded)
    if(TARGET ${target})
        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/core
            ${CMAKE_SOURCE_DIR}/models
            ${CMAKE_SOURCE_DIR}/utils
            ${CMAKE_SOURCE_DIR}/interface
        )
    endif()
endforeach()

# Memory analysis target
add_custom_target(embedded-memory-analysis
    COMMAND ${CMAKE_COMMAND} -E echo "=== Embedded Memory Analysis ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Memory Usage Estimates:"
    COMMAND ${CMAKE_COMMAND} -E echo "Arduino (32KB limit):"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Code: ~20KB"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Model: ~8KB (500 vocab, 32 hidden, 2 layers)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Runtime: ~4KB"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Raspberry Pi (64MB limit):"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Code: ~100KB"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Model: ~2MB (2000 vocab, 64 hidden, 4 layers)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Runtime: ~4MB"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Generic Embedded (16MB limit):"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Code: ~80KB"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Model: ~1MB (1000 vocab, 48 hidden, 3 layers)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Runtime: ~2MB"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Analyzing embedded memory requirements"
)

# Performance testing for embedded
add_custom_target(embedded-perf-test
    COMMAND ${CMAKE_COMMAND} -E echo "=== Embedded Performance Testing ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Expected Performance (tokens/second):"
    COMMAND ${CMAKE_COMMAND} -E echo "Arduino (16MHz): ~0.1-0.5 tokens/sec"
    COMMAND ${CMAKE_COMMAND} -E echo "Raspberry Pi Zero: ~1-5 tokens/sec"
    COMMAND ${CMAKE_COMMAND} -E echo "Raspberry Pi 4: ~10-50 tokens/sec"
    COMMAND ${CMAKE_COMMAND} -E echo "Generic ARM Cortex-M4: ~0.5-2 tokens/sec"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Note: Performance depends on model size and optimization level"
    COMMENT "Displaying embedded performance expectations"
)

# Cross-compilation helpers
add_custom_target(embedded-toolchains
    COMMAND ${CMAKE_COMMAND} -E echo "=== Embedded Cross-Compilation Toolchains ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Arduino:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Use Arduino IDE or PlatformIO"
    COMMAND ${CMAKE_COMMAND} -E echo "  Add Hyperion library to libraries folder"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Raspberry Pi:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Native compilation: cmake .. && make"
    COMMAND ${CMAKE_COMMAND} -E echo "  Cross-compilation: cmake .. -DCMAKE_TOOLCHAIN_FILE=arm-linux-gnueabihf.cmake"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Generic ARM:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Use appropriate GCC ARM toolchain"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .. -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi.cmake"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying cross-compilation information"
)

# Installation targets for embedded
if(PLATFORM_ARDUINO)
    install(TARGETS hyperion_arduino
            ARCHIVE DESTINATION lib/arduino
            COMPONENT embedded)
    install(FILES hyperion_arduino.h
            DESTINATION include/arduino
            COMPONENT embedded)
elseif(PLATFORM_RASPBERRY_PI)
    install(TARGETS hyperion_rpi
            ARCHIVE DESTINATION lib/rpi
            COMPONENT embedded)
    install(FILES hyperion_raspberry_pi.h
            DESTINATION include/rpi
            COMPONENT embedded)
else()
    install(TARGETS hyperion_embedded
            ARCHIVE DESTINATION lib/embedded
            COMPONENT embedded)
    install(FILES hyperion_generic_embedded.h
            DESTINATION include/embedded
            COMPONENT embedded)
endif()

# Documentation target
add_custom_target(embedded-docs
    COMMAND ${CMAKE_COMMAND} -E echo "=== Embedded Documentation ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Quick Start Guides:"
    COMMAND ${CMAKE_COMMAND} -E echo "  docs/embedded/arduino-quickstart.md"
    COMMAND ${CMAKE_COMMAND} -E echo "  docs/embedded/raspberry-pi-setup.md"
    COMMAND ${CMAKE_COMMAND} -E echo "  docs/embedded/optimization-guide.md"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "API References:"
    COMMAND ${CMAKE_COMMAND} -E echo "  docs/api/embedded-api.md"
    COMMAND ${CMAKE_COMMAND} -E echo "  docs/api/memory-management.md"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Examples:"
    COMMAND ${CMAKE_COMMAND} -E echo "  examples/embedded/arduino-chatbot/"
    COMMAND ${CMAKE_COMMAND} -E echo "  examples/embedded/rpi-voice-assistant/"
    COMMAND ${CMAKE_COMMAND} -E echo "  examples/embedded/sensor-data-analysis/"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying embedded documentation links"
)

# Build instructions and usage
add_custom_target(embedded-help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Hyperion Embedded Systems Build ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Supported Platforms:"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Arduino (ATmega, ESP32, ARM Cortex-M)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Raspberry Pi (Zero, 3, 4, CM4)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Generic ARM embedded systems"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Microcontrollers with 32KB+ RAM"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build Commands:"
    COMMAND ${CMAKE_COMMAND} -E echo "  # Raspberry Pi"
    COMMAND ${CMAKE_COMMAND} -E echo "  mkdir build-rpi && cd build-rpi"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel"
    COMMAND ${CMAKE_COMMAND} -E echo "  make -j4"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  # Arduino (use Arduino IDE or PlatformIO)"
    COMMAND ${CMAKE_COMMAND} -E echo "  # Add Hyperion library to your project"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Features:"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Ultra-low memory footprint (32KB+ RAM)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - 4-bit quantization for minimal storage"
    COMMAND ${CMAKE_COMMAND} -E echo "  - SIMD optimization (NEON, SSE)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - No dynamic memory allocation"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Real-time inference capability"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Use Cases:"
    COMMAND ${CMAKE_COMMAND} -E echo "  - IoT devices with AI capabilities"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Smart sensors and actuators"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Edge AI for industrial automation"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Battery-powered AI devices"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying embedded systems build help"
)