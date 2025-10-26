# WebAssembly Build Configuration for Hyperion
cmake_minimum_required(VERSION 3.13)

# WASM-specific configuration
if(EMSCRIPTEN)
    message(STATUS "Building Hyperion for WebAssembly with Emscripten")
    
    # Set WASM-specific flags
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    # Emscripten-specific compiler flags
    set(WASM_COMPILE_FLAGS 
        "-s WASM=1"
        "-s EXPORTED_FUNCTIONS=['_hyperion_wasm_init','_hyperion_wasm_generate_text','_hyperion_wasm_cleanup','_malloc','_free']"
        "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','UTF8ToString','getValue','setValue']"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s INITIAL_MEMORY=16MB"
        "-s MAXIMUM_MEMORY=64MB"
        "-s STACK_SIZE=1MB"
        "-s MODULARIZE=1"
        "-s EXPORT_NAME='HyperionModule'"
        "-s ENVIRONMENT='web,worker'"
        "-s FILESYSTEM=0"
        "-s DISABLE_EXCEPTION_CATCHING=1"
        "-s ASSERTIONS=0"
        "-O3"
        "--closure 1"
        "--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/hyperion_pre.js"
        "--post-js ${CMAKE_CURRENT_SOURCE_DIR}/hyperion_post.js"
    )
    
    # Debug vs Release flags
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND WASM_COMPILE_FLAGS 
            "-s ASSERTIONS=1"
            "-s SAFE_HEAP=1"
            "-g"
        )
    else()
        list(APPEND WASM_COMPILE_FLAGS 
            "-s ASSERTIONS=0"
            "--closure 1"
            "-flto"
        )
    endif()
    
    # Apply flags
    string(REPLACE ";" " " WASM_FLAGS_STR "${WASM_COMPILE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WASM_FLAGS_STR}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${WASM_FLAGS_STR}")
    
    # WASM-specific source files
    set(HYPERION_WASM_SOURCES
        hyperion_wasm_interface.c
        wasm_memory_manager.c
        wasm_performance_optimizer.c
    )
    
    # Core Hyperion sources (adapted for WASM)
    set(HYPERION_CORE_SOURCES
        ../../core/memory.c
        ../../core/config.c
        ../../models/text/generate.c
        ../../models/text/tokenizer.c
    )
    
    # Create WASM library
    add_executable(hyperion_wasm
        ${HYPERION_WASM_SOURCES}
        ${HYPERION_CORE_SOURCES}
    )
    
    target_include_directories(hyperion_wasm PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/models
        ${CMAKE_SOURCE_DIR}/interface
    )
    
    # WASM-specific preprocessor definitions
    target_compile_definitions(hyperion_wasm PRIVATE
        HYPERION_WASM=1
        HYPERION_NO_FILESYSTEM=1
        HYPERION_MINIMAL_MEMORY=1
        HYPERION_BROWSER_MODE=1
    )
    
    # Custom target for optimized WASM build
    add_custom_target(wasm-optimized
        COMMAND ${CMAKE_COMMAND} --build . --config Release
        COMMAND wasm-opt -O3 -o hyperion_wasm_optimized.wasm hyperion_wasm.wasm
        DEPENDS hyperion_wasm
        COMMENT "Building optimized WASM binary"
    )
    
    # Custom target for WASM testing
    add_custom_target(wasm-test
        COMMAND node test_hyperion_wasm.js
        DEPENDS hyperion_wasm
        COMMENT "Testing WASM build with Node.js"
    )
    
    # Installation for web deployment
    install(FILES 
        ${CMAKE_CURRENT_BINARY_DIR}/hyperion_wasm.js
        ${CMAKE_CURRENT_BINARY_DIR}/hyperion_wasm.wasm
        ${CMAKE_CURRENT_SOURCE_DIR}/hyperion_web_demo.html
        ${CMAKE_CURRENT_SOURCE_DIR}/hyperion_worker.js
        DESTINATION web/
        COMPONENT wasm
    )
    
else()
    message(WARNING "WASM build requires Emscripten. Install Emscripten SDK and set EMSCRIPTEN=1")
endif()

# Build instructions and usage
add_custom_target(wasm-help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Hyperion WebAssembly Build ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Prerequisites:"
    COMMAND ${CMAKE_COMMAND} -E echo "  1. Install Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html"
    COMMAND ${CMAKE_COMMAND} -E echo "  2. Activate Emscripten: source ./emsdk_env.sh"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build Commands:"
    COMMAND ${CMAKE_COMMAND} -E echo "  emcmake cmake -B build-wasm -DEMSCRIPTEN=ON"
    COMMAND ${CMAKE_COMMAND} -E echo "  emmake cmake --build build-wasm"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Optimization:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build build-wasm --target wasm-optimized"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Testing:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build build-wasm --target wasm-test"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Web Deployment:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --install build-wasm --component wasm"
    COMMAND ${CMAKE_COMMAND} -E echo "  Serve files from web/ directory with HTTP server"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying WASM build help"
)