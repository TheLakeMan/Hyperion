# Android NDK Build Configuration for Hyperion
cmake_minimum_required(VERSION 3.18.1)

project(hyperion_android)

# Android-specific settings
if(ANDROID)
    message(STATUS "Building Hyperion for Android with NDK")
    
    # Set Android API level and architecture
    set(ANDROID_PLATFORM android-21)
    set(ANDROID_STL c++_shared)
    
    # Find required Android libraries
    find_library(log-lib log)
    find_library(android-lib android)
    find_library(jnigraphics-lib jnigraphics)
    
    # Android-specific compiler flags
    set(ANDROID_COMPILE_FLAGS
        "-DANDROID"
        "-DHYPERION_ANDROID=1"
        "-DHYPERION_MOBILE=1"
        "-DHYPERION_NO_FILESYSTEM=1"
        "-ffast-math"
        "-ftree-vectorize"
        "-fomit-frame-pointer"
    )
    
    # Architecture-specific optimizations
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        list(APPEND ANDROID_COMPILE_FLAGS "-march=armv8-a" "-mtune=cortex-a75")
        set(HYPERION_SIMD_SUPPORT ON)
        add_definitions(-DHYPERION_NEON=1)
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        list(APPEND ANDROID_COMPILE_FLAGS "-march=armv7-a" "-mfpu=neon" "-mfloat-abi=softfp")
        set(HYPERION_SIMD_SUPPORT ON)
        add_definitions(-DHYPERION_NEON=1)
    elseif(ANDROID_ABI STREQUAL "x86_64")
        list(APPEND ANDROID_COMPILE_FLAGS "-march=x86-64" "-msse4.2")
        set(HYPERION_SIMD_SUPPORT ON)
        add_definitions(-DHYPERION_SSE=1)
    elseif(ANDROID_ABI STREQUAL "x86")
        list(APPEND ANDROID_COMPILE_FLAGS "-march=i686" "-msse2")
        set(HYPERION_SIMD_SUPPORT ON)
        add_definitions(-DHYPERION_SSE=1)
    endif()
    
    # Apply compiler flags
    string(REPLACE ";" " " ANDROID_FLAGS_STR "${ANDROID_COMPILE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ANDROID_FLAGS_STR}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ANDROID_FLAGS_STR}")
    
    # Android-specific source files
    set(HYPERION_ANDROID_SOURCES
        hyperion_android_jni.c
        ../../../core/memory.c
        ../../../core/config.c
        ../../../models/text/generate.c
        ../../../models/text/tokenizer.c
        ../../../utils/tensor.c
    )
    
    # Create Android shared library
    add_library(hyperion_android SHARED
        ${HYPERION_ANDROID_SOURCES}
    )
    
    target_include_directories(hyperion_android PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/models
        ${CMAKE_SOURCE_DIR}/utils
        ${CMAKE_SOURCE_DIR}/interface
    )
    
    # Android-specific preprocessor definitions
    target_compile_definitions(hyperion_android PRIVATE
        ANDROID_NDK=1
        HYPERION_ANDROID=1
        HYPERION_MOBILE=1
        HYPERION_MINIMAL_MEMORY=1
        HYPERION_4BIT_QUANTIZATION=1
    )
    
    # Link Android libraries
    target_link_libraries(hyperion_android
        ${log-lib}
        ${android-lib}
        ${jnigraphics-lib}
        m  # Math library
    )
    
    # Set output name and properties
    set_target_properties(hyperion_android PROPERTIES
        OUTPUT_NAME "hyperion"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ABI}"
    )
    
    # Custom target for APK integration
    add_custom_target(android-integration
        COMMAND ${CMAKE_COMMAND} -E echo "=== Android Integration Instructions ==="
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "1. Copy libhyperion.so to your Android project:"
        COMMAND ${CMAKE_COMMAND} -E echo "   app/src/main/jniLibs/${ANDROID_ABI}/libhyperion.so"
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "2. Add to your build.gradle:"
        COMMAND ${CMAKE_COMMAND} -E echo "   android {"
        COMMAND ${CMAKE_COMMAND} -E echo "     packagingOptions {"
        COMMAND ${CMAKE_COMMAND} -E echo "       pickFirst '**/libhyperion.so'"
        COMMAND ${CMAKE_COMMAND} -E echo "     }"
        COMMAND ${CMAKE_COMMAND} -E echo "   }"
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "3. Load library in Java:"
        COMMAND ${CMAKE_COMMAND} -E echo "   static {"
        COMMAND ${CMAKE_COMMAND} -E echo "     System.loadLibrary(\"hyperion\");"
        COMMAND ${CMAKE_COMMAND} -E echo "   }"
        DEPENDS hyperion_android
        COMMENT "Displaying Android integration instructions"
    )
    
    # Performance testing target
    add_custom_target(android-perf-test
        COMMAND adb push $<TARGET_FILE:hyperion_android> /data/local/tmp/
        COMMAND adb shell "/data/local/tmp/libhyperion.so || echo 'Manual testing required'"
        DEPENDS hyperion_android
        COMMENT "Pushing library to Android device for testing"
    )
    
else()
    message(WARNING "Android NDK build requires ANDROID=ON and proper NDK toolchain")
endif()

# Build instructions and usage
add_custom_target(android-help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Hyperion Android NDK Build ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Prerequisites:"
    COMMAND ${CMAKE_COMMAND} -E echo "  1. Install Android NDK: https://developer.android.com/ndk/downloads"
    COMMAND ${CMAKE_COMMAND} -E echo "  2. Set ANDROID_NDK environment variable"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build Commands:"
    COMMAND ${CMAKE_COMMAND} -E echo "  mkdir build-android && cd build-android"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .. -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \\"
    COMMAND ${CMAKE_COMMAND} -E echo "           -DANDROID_ABI=arm64-v8a \\"
    COMMAND ${CMAKE_COMMAND} -E echo "           -DANDROID_PLATFORM=android-21"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build ."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Supported ABIs:"
    COMMAND ${CMAKE_COMMAND} -E echo "  - arm64-v8a (recommended for modern devices)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - armeabi-v7a (older ARM devices)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - x86_64 (emulators and x86 devices)"
    COMMAND ${CMAKE_COMMAND} -E echo "  - x86 (older emulators)"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying Android NDK build help"
)