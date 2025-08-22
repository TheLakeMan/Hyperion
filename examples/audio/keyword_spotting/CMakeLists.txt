# Keyword Spotting Example

# Source files
set(KEYWORD_SPOTTING_SOURCES
    main.c
    kws.c
)

# Create executable
add_executable(hyperion_kws ${KEYWORD_SPOTTING_SOURCES})

# Include directories
target_include_directories(hyperion_kws PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link libraries
target_link_libraries(hyperion_kws PRIVATE
    hyperion_core
    hyperion_models
    hyperion_utils
)

# Set optimization flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hyperion_kws PRIVATE -O2)
elseif(MSVC)
    target_compile_options(hyperion_kws PRIVATE /O2)
endif()

# Install executable
install(TARGETS hyperion_kws DESTINATION bin)

# Install sample data
install(FILES
    ${CMAKE_SOURCE_DIR}/data/sample_speech.wav
    DESTINATION share/hyperion/examples/audio
)
