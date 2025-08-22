# Voice Activity Detection Example

# Source files
set(VOICE_DETECTION_SOURCES
    main.c
    vad.c
)

# Create executable
add_executable(hyperion_vad ${VOICE_DETECTION_SOURCES})

# Include directories
target_include_directories(hyperion_vad PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link libraries
target_link_libraries(hyperion_vad PRIVATE
    hyperion_core
    hyperion_models
    hyperion_utils
)

# Set optimization flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hyperion_vad PRIVATE -O2)
elseif(MSVC)
    target_compile_options(hyperion_vad PRIVATE /O2)
endif()

# Install executable
install(TARGETS hyperion_vad DESTINATION bin)

# Install sample data
install(FILES
    ${CMAKE_SOURCE_DIR}/data/sample_speech.wav
    DESTINATION share/hyperion/examples/audio
)
