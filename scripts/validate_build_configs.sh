#!/bin/bash

# Hyperion Build Configuration Validation Script
# Tests all CMake presets and build configurations across platforms

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_BASE_DIR="$PROJECT_DIR/build-validation"
LOG_DIR="$BUILD_BASE_DIR/logs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Logging functions
log() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_DIR/validation.log"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1" | tee -a "$LOG_DIR/validation.log"
    ((PASSED_TESTS++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1" | tee -a "$LOG_DIR/validation.log"
    ((FAILED_TESTS++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1" | tee -a "$LOG_DIR/validation.log"
}

# Setup function
setup_validation() {
    log "Setting up build validation environment..."
    
    # Create directories
    mkdir -p "$BUILD_BASE_DIR"
    mkdir -p "$LOG_DIR"
    
    # Clear previous logs
    > "$LOG_DIR/validation.log"
    
    # Print system information
    log "System Information:"
    log "  OS: $(uname -s)"
    log "  Architecture: $(uname -m)"
    log "  CMake version: $(cmake --version | head -n1)"
    log "  Compiler: $(gcc --version | head -n1 2>/dev/null || echo 'GCC not found')"
    log "  Clang: $(clang --version | head -n1 2>/dev/null || echo 'Clang not found')"
    
    cd "$PROJECT_DIR"
}

# Test individual build configuration
test_build_config() {
    local config_name="$1"
    local cmake_args="$2"
    local build_dir="$BUILD_BASE_DIR/$config_name"
    
    ((TOTAL_TESTS++))
    
    log "Testing build configuration: $config_name"
    
    # Clean previous build
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    
    # Configure
    log "  Configuring with: $cmake_args"
    if cmake -B "$build_dir" $cmake_args > "$LOG_DIR/$config_name-configure.log" 2>&1; then
        log "  ✓ Configuration successful"
    else
        log_error "Configuration failed for $config_name"
        log "    See $LOG_DIR/$config_name-configure.log for details"
        return 1
    fi
    
    # Build
    log "  Building..."
    if cmake --build "$build_dir" > "$LOG_DIR/$config_name-build.log" 2>&1; then
        log "  ✓ Build successful"
    else
        log_error "Build failed for $config_name"
        log "    See $LOG_DIR/$config_name-build.log for details"
        return 1
    fi
    
    # Test if executables were created
    if [ -f "$build_dir/examples/beginner_hello_world" ] || [ -f "$build_dir/examples/beginner_hello_world.exe" ]; then
        log "  ✓ Example executables created"
    else
        log_warning "Example executables not found for $config_name"
    fi
    
    log_success "Build configuration $config_name completed successfully"
    return 0
}

# Test CMake presets
test_cmake_presets() {
    log "Testing CMake presets..."
    
    # Get list of available presets
    local presets
    if command -v jq >/dev/null 2>&1; then
        presets=$(jq -r '.configurePresets[].name' CMakePresets.json 2>/dev/null || echo "")
    else
        # Fallback: extract preset names manually
        presets=$(grep -o '"name"[^"]*"[^"]*"' CMakePresets.json | cut -d'"' -f4 2>/dev/null || echo "")
    fi
    
    if [ -z "$presets" ]; then
        log_warning "No CMake presets found or could not parse CMakePresets.json"
        return 0
    fi
    
    for preset in $presets; do
        ((TOTAL_TESTS++))
        log "Testing preset: $preset"
        
        local build_dir="$BUILD_BASE_DIR/preset-$preset"
        rm -rf "$build_dir"
        
        if cmake --preset="$preset" > "$LOG_DIR/preset-$preset.log" 2>&1; then
            if cmake --build "$build_dir" >> "$LOG_DIR/preset-$preset.log" 2>&1; then
                log_success "Preset $preset built successfully"
            else
                log_error "Preset $preset build failed"
            fi
        else
            log_error "Preset $preset configuration failed"
        fi
    done
}

# Test different build types
test_build_types() {
    log "Testing different build types..."
    
    local build_types=("Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    
    for build_type in "${build_types[@]}"; do
        test_build_config "build-type-$build_type" "-DCMAKE_BUILD_TYPE=$build_type -DBUILD_EXAMPLES=ON"
    done
}

# Test compiler variations
test_compilers() {
    log "Testing different compilers..."
    
    # Test GCC if available
    if command -v gcc >/dev/null 2>&1; then
        test_build_config "gcc-build" "-DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON"
    else
        log_warning "GCC not available, skipping GCC test"
    fi
    
    # Test Clang if available
    if command -v clang >/dev/null 2>&1; then
        test_build_config "clang-build" "-DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON"
    else
        log_warning "Clang not available, skipping Clang test"
    fi
}

# Test feature combinations
test_feature_combinations() {
    log "Testing feature combinations..."
    
    # Test with all features enabled
    test_build_config "all-features" "-DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON -DENABLE_SIMD=ON"
    
    # Test minimal build
    test_build_config "minimal" "-DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF"
    
    # Test debug with memory tracking
    test_build_config "debug-memory" "-DCMAKE_BUILD_TYPE=Debug -DENABLE_MEMORY_DEBUGGING=ON -DBUILD_TESTS=ON"
}

# Test quick build scripts
test_quick_build_scripts() {
    log "Testing quick build scripts..."
    
    # Test Unix/Linux script
    if [ -f "build_quick.sh" ]; then
        ((TOTAL_TESTS++))
        log "Testing build_quick.sh..."
        chmod +x build_quick.sh
        
        if ./build_quick.sh > "$LOG_DIR/quick-build-sh.log" 2>&1; then
            log_success "build_quick.sh executed successfully"
        else
            log_error "build_quick.sh failed"
        fi
    fi
    
    # Test Windows script (if running on Windows/WSL)
    if [ -f "build_quick.bat" ] && command -v cmd.exe >/dev/null 2>&1; then
        ((TOTAL_TESTS++))
        log "Testing build_quick.bat..."
        
        if cmd.exe /c build_quick.bat > "$LOG_DIR/quick-build-bat.log" 2>&1; then
            log_success "build_quick.bat executed successfully"
        else
            log_error "build_quick.bat failed"
        fi
    fi
}

# Test cross-compilation (if tools available)
test_cross_compilation() {
    log "Testing cross-compilation capabilities..."
    
    # ARM64 cross-compilation
    if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
        test_build_config "cross-arm64" "-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF"
    else
        log_warning "ARM64 cross-compiler not available"
    fi
    
    # ARM32 cross-compilation
    if command -v arm-linux-gnueabihf-gcc >/dev/null 2>&1; then
        test_build_config "cross-arm32" "-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF"
    else
        log_warning "ARM32 cross-compiler not available"
    fi
}

# Run integration tests
test_integration() {
    log "Running integration tests..."
    
    # Use the most recent successful build
    local build_dir="$BUILD_BASE_DIR/build-type-Release"
    
    if [ ! -d "$build_dir" ]; then
        log_warning "No release build available for integration testing"
        return 0
    fi
    
    cd "$build_dir"
    
    # Test example execution
    if [ -f "examples/beginner_hello_world" ]; then
        ((TOTAL_TESTS++))
        log "Testing beginner example execution..."
        
        if timeout 30s ./examples/beginner_hello_world > "$LOG_DIR/integration-beginner.log" 2>&1; then
            log_success "Beginner example executed successfully"
        else
            log_error "Beginner example execution failed or timed out"
        fi
    fi
    
    # Test memory validation if available
    if [ -f "tests/memory_validation_suite" ]; then
        ((TOTAL_TESTS++))
        log "Running memory validation tests..."
        
        if timeout 120s ./tests/memory_validation_suite > "$LOG_DIR/integration-memory.log" 2>&1; then
            log_success "Memory validation tests passed"
        else
            log_error "Memory validation tests failed"
        fi
    fi
    
    cd "$PROJECT_DIR"
}

# Generate final report
generate_report() {
    log "Generating validation report..."
    
    local report_file="$LOG_DIR/validation-report.txt"
    
    cat > "$report_file" << EOF
=== Hyperion Build Validation Report ===
Generated: $(date)
Project Directory: $PROJECT_DIR

=== Summary ===
Total Tests: $TOTAL_TESTS
Passed: $PASSED_TESTS
Failed: $FAILED_TESTS
Success Rate: $(echo "scale=1; $PASSED_TESTS * 100 / $TOTAL_TESTS" | bc -l 2>/dev/null || echo "N/A")%

=== System Information ===
OS: $(uname -s)
Architecture: $(uname -m)
CMake: $(cmake --version | head -n1)
GCC: $(gcc --version | head -n1 2>/dev/null || echo 'Not available')
Clang: $(clang --version | head -n1 2>/dev/null || echo 'Not available')

=== Detailed Results ===
See individual log files in $LOG_DIR/ for detailed information:
- validation.log: Complete execution log
- *-configure.log: Configuration phase logs
- *-build.log: Build phase logs
- integration-*.log: Integration test logs

=== Recommendations ===
EOF

    if [ $FAILED_TESTS -eq 0 ]; then
        echo "✅ All tests passed! The build system is working correctly." >> "$report_file"
    elif [ $FAILED_TESTS -lt 3 ]; then
        echo "⚠️  Some tests failed, but overall build system appears functional." >> "$report_file"
        echo "   Review failed test logs for specific issues." >> "$report_file"
    else
        echo "❌ Multiple test failures detected. Build system needs attention." >> "$report_file"
        echo "   Priority: Fix configuration and compilation issues first." >> "$report_file"
    fi
    
    cat "$report_file"
    log "Validation report saved to: $report_file"
}

# Main execution
main() {
    log "=== Hyperion Build Configuration Validation ==="
    log "Starting comprehensive build validation..."
    
    setup_validation
    
    # Run all validation tests
    test_cmake_presets
    test_build_types
    test_compilers
    test_feature_combinations
    test_quick_build_scripts
    test_cross_compilation
    test_integration
    
    # Generate final report
    generate_report
    
    # Exit with appropriate code
    if [ $FAILED_TESTS -eq 0 ]; then
        log_success "=== All validation tests completed successfully! ==="
        exit 0
    else
        log_error "=== Validation completed with $FAILED_TESTS failed tests ==="
        exit 1
    fi
}

# Run main function
main "$@"