# Speech Recognition Example

# Source files
set(SPEECH_RECOGNITION_SOURCES
    main.c
    asr.c
)

# Create executable
add_executable(hyperion_asr ${SPEECH_RECOGNITION_SOURCES})

# Include directories
target_include_directories(hyperion_asr PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link libraries
target_link_libraries(hyperion_asr PRIVATE
    hyperion_core
    hyperion_models
    hyperion_utils
)

# Set optimization flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hyperion_asr PRIVATE -O2)
elseif(MSVC)
    target_compile_options(hyperion_asr PRIVATE /O2)
endif()

# Install executable
install(TARGETS hyperion_asr DESTINATION bin)

# Install sample data
install(FILES
    ${CMAKE_SOURCE_DIR}/data/sample_speech.wav
    DESTINATION share/hyperion/examples/audio
)
