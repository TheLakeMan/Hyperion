# Qoder.md - Hyperion Project Context

## Project Context

**Project Name:** Hyperion  
**Repository:** https://github.com/TheLakeMan/Hyperion  
**Project Root:** C:\Users\verme\Desktop\Hyperion\Hyperion  
**GitHub Username:** TheLakeMan  

### Project Overview
Hyperion is an ultra-lightweight AI framework designed to run efficiently on minimal hardware, including legacy systems. It emphasizes extreme memory efficiency through 4-bit quantization and sparse matrix representations, enabling AI inference on devices with as little as 50–100MB RAM.

### Key Features
- **4-bit Quantization:** Reduces model size up to 8x
- **Hybrid Execution:** Model Context Protocol (MCP) for local/remote processing  
- **SIMD Optimization:** AVX2, AVX, SSE acceleration
- **Cross-platform:** Pure C implementation (Windows, Linux, macOS)
- **Memory Efficient:** Advanced memory pooling and sparse operations
- **Web Interface:** RESTful API with WebSocket support for real-time streaming
- **Performance Monitoring:** Comprehensive profiling and benchmarking tools

## Development Workflow

### Build Commands
```bash
# Windows (Developer Command Prompt)
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
ctest -C Release

# Linux/macOS
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

### Environment Setup
- **OS:** Windows 22H2
- **IDE:** Qoder IDE 0.1.15
- **Compiler:** MSVC (Visual Studio 2022)
- **Build System:** CMake 3.10+

### File Management Preferences
- Use `search_replace` over `edit_file` for code modifications
- Update status every 5-10 minutes during development
- Use `task_log.md` for tracking tasks and completion times
- Consolidate overlapping documentation into single files

### MCP Protocol
- MCP stands for Model Context Protocol
- Used for hybrid local/remote execution capabilities
- Environment variable setup: `set GH_TOKEN=<token> && gh <command>`

## Key Project Facts

### Current Status
- **Phase:** Feature-complete framework with enhanced web capabilities
- **Focus:** Performance optimization and real-time streaming interfaces
- **Recent Additions:** WebSocket support, enhanced REST API, performance monitoring
- **Testing:** Comprehensive test suite in `/tests` directory
- **Examples:** Multiple domain examples (audio, image, text, multimodal)

### Architecture
1. **Core Layer:** Picol interpreter, memory management, I/O, configuration
2. **Model Layer:** Quantized models (text, image, audio, multimodal)
3. **Interface Layer:** CLI, web server with WebSocket support, scripting shell
4. **Hybrid Layer:** MCP client for remote execution integration
5. **Monitoring Layer:** Performance profiling and memory usage tracking

### Project Evolution
- **Original Name:** TinyAI (fully migrated to Hyperion)
- **Current Version:** v0.1.0
- **Repository Migration:** Completed from original TinyAI codebase

### Documentation Structure
Main documentation files in project root:- **README.md:** Main project overview and quick start guide
- **CONTRIBUTING.md:** Development guidelines and contribution process
- **ARCHITECTURE_ROADMAP.md:** System architecture and future plans
- **BUILD_STATUS.md:** Current build status and compatibility
- **IMPLEMENTATION_STATUS.md:** Feature implementation progress
- **TECHNICAL_DOCUMENTATION.md:** Detailed technical specifications

### Useful Commands
```bash
# Priority test execution
.\run_priority_tests.bat        # Windows
./run_priority_tests.sh         # Linux/macOS

# Build cleanup
.\cleanup_build.bat             # Windows
./cleanup_build.ps1             # PowerShell

# Visual Studio setup
.\test_vs_setup.bat             # Test VS environment

# Web server build and test
.\build_webserver.bat           # Build web server with WebSocket support
# Then open: http://localhost:8080 or http://localhost:8080/websocket_test.html
```

### Development Notes
- **Memory Management:** Advanced memory pooling with leak detection
- **Performance:** SIMD optimizations (SSE2, AVX, AVX2) enabled by default
- **Testing:** Comprehensive test suite covering all major components
- **Cross-platform:** Windows, Linux, macOS support with minimal dependencies

### Git Workflow
For complex conflicts or diverged history:
1. Reset local `main` branch to match `origin/main`
2. Create new, clean feature branch
3. Re-apply necessary changes to new branch
4. Push new branch for clean pull request

---

*This document serves as the primary project context reference for the Hyperion AI framework development environment.*]
