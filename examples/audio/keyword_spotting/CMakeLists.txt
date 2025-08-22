# Keyword Spotting Example

# Source files
set(KEYWORD_SPOTTING_SOURCES
    main.c
    kws.c
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
)

# Create executable
add_executable(hyperion_kws ${KEYWORD_SPOTTING_SOURCES})

# Link against TinyAI library
target_link_libraries(hyperion_kws hyperion)

# Install executable
install(TARGETS hyperion_kws DESTINATION bin)

# Install sample data
install(FILES
    ${CMAKE_SOURCE_DIR}/data/sample_speech.wav
    DESTINATION share/hyperion/examples/audio
)
