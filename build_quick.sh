#!/bin/bash
# Hyperion AI Framework - Quick Build Script for Linux/macOS
# This script performs a fast development build with basic testing

set -e  # Exit on any error

echo "========================================"
echo "Hyperion AI Framework - Quick Build"
echo "========================================"

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run from project root."
    exit 1
fi

# Detect OS
OS=$(uname -s)
case $OS in
    Linux*)
        echo "Detected: Linux"
        PRESET="linux-release"
        ;;
    Darwin*)
        echo "Detected: macOS"
        PRESET="macos-release"
        ;;
    *)
        echo "Detected: $OS (using generic Unix settings)"
        PRESET="linux-release"
        ;;
esac

# Check for required tools
command -v cmake >/dev/null 2>&1 || {
    echo "Error: cmake is required but not installed."
    echo "Ubuntu/Debian: sudo apt-get install cmake"
    echo "macOS: brew install cmake"
    exit 1
}

command -v gcc >/dev/null 2>&1 || command -v clang >/dev/null 2>&1 || {
    echo "Error: gcc or clang is required but not installed."
    echo "Ubuntu/Debian: sudo apt-get install build-essential"
    echo "macOS: xcode-select --install"
    exit 1
}

# Clean previous build
echo "Cleaning previous build..."
rm -rf build/
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
if command -v cmake >/dev/null 2>&1 && cmake --version | grep -q "version 3.2[0-9]\|version 3.1[9-9]"; then
    # Use presets if CMake 3.19+
    cmake --preset="$PRESET" ..
else
    # Fallback for older CMake versions
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DHYPERION_BUILD_TESTS=ON \
          -DHYPERION_BUILD_EXAMPLES=ON \
          -DHYPERION_ENABLE_SIMD=ON ..
fi

# Build the project
echo "Building project..."
NUM_CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "Using $NUM_CORES parallel jobs"
cmake --build . --parallel $NUM_CORES

# Run quick tests
echo "Running quick tests..."
if ctest --output-on-failure --timeout 60; then
    echo "All tests passed!"
else
    echo "Warning: Some tests failed. Check output above."
fi

cd ..

echo "========================================"
echo "Quick build completed successfully!"
echo "========================================"
echo
echo "You can now:"
echo "  - Run CLI: ./build/hyperion --help"
echo "  - Start web server: ./build/interface/web_server"
echo "  - Run all tests: ./run_priority_tests.sh"
echo "  - See examples: ./examples/"
echo

# Make the script executable
chmod +x build_quick.sh 2>/dev/null || true