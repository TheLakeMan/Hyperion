# Hyperion AI Framework - Quick Start Guide

*Get up and running with Hyperion in 5 minutes!*

## 🚀 **5-Minute Setup**

### Step 1: Prerequisites (1 minute)

#### Windows
```bash
# Install Visual Studio 2022 Community (if not installed)
# Download from: https://visualstudio.microsoft.com/downloads/

# Verify installation
where cmake
where git
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake git -y
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install cmake git
```

### Step 2: Clone & Build (2 minutes)

```bash
# Clone repository
git clone https://github.com/TheLakeMan/Hyperion.git
cd Hyperion

# Quick build test (Windows)
.\test_vs_setup.bat
.\build_webserver.bat

# Quick build test (Linux/macOS)
./run_priority_tests.sh
mkdir build && cd build
cmake .. && cmake --build .
```

### Step 3: First Run (1 minute)

```bash
# Test CLI interface
./build/hyperion --help

# Start web server (Windows)
.\build_webserver.bat

# Start web server (Linux/macOS)  
./build/interface/web_server

# Open browser to: http://localhost:8080
```

### Step 4: Verify Installation (1 minute)

```bash
# Run quick tests
./build/tests/test_main

# Check memory usage
./build/tests/test_memory

# Performance check
./build/tests/performance_benchmark_suite --quick
```

**✅ Success!** You should see:
- ✓ All tests passing
- ✓ Web server running on port 8080
- ✓ Memory usage <100MB for basic operations

---

## 🎡 **First Examples**

### Text Generation (Beginner)

```bash
# Interactive text generation
./build/hyperion generate --prompt "Hello, my name is" --max-tokens 50

# Example output:
# "Hello, my name is Alex and I'm excited to try Hyperion AI!"
```

### Web API Usage

```bash
# Start web server
./build/interface/web_server &

# Test API endpoint
curl -X POST http://localhost:8080/api/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "The weather today is", "max_tokens": 20}'
```

### Web Interface Demo

1. **Start server**: `./build_webserver.bat` (Windows) or `./build/interface/web_server` (Linux/macOS)
2. **Open browser**: Navigate to `http://localhost:8080`
3. **Try WebSocket demo**: Go to `http://localhost:8080/websocket_test.html`
4. **Enter prompt**: Type any text and click "Generate"
5. **Watch real-time streaming**: See text generated live!

---

## 🛠️ **Configuration**

### Basic Configuration

Create `config.json` in your project root:

```json
{
  "model": {
    "type": "text",
    "quantization": "4bit",
    "max_context_length": 2048
  },
  "memory": {
    "pool_size_mb": 128,
    "enable_profiling": true
  },
  "server": {
    "port": 8080,
    "host": "localhost",
    "enable_websocket": true
  }
}
```

### Environment Variables

```bash
# Optional optimizations
export HYPERION_SIMD_ENABLED=1
export HYPERION_LOG_LEVEL=INFO
export HYPERION_MEMORY_POOL_SIZE=134217728  # 128MB
```

---

## 📚 **Common Use Cases**

### 1. **Chatbot Development**
```bash
# Run chatbot example
cd examples/chatbot
cmake --build . && ./chatbot

# Interactive chat session starts
```

### 2. **Image Recognition**
```bash
# Process an image
./build/hyperion classify --image path/to/image.jpg

# Batch processing
./build/hyperion classify --batch path/to/images/ --output results.json
```

### 3. **Audio Processing**
```bash
# Speech recognition
./build/hyperion transcribe --audio recording.wav

# Voice activity detection
./build/hyperion detect-voice --audio stream.wav --threshold 0.5
```

### 4. **Multimodal Processing**
```bash
# Image captioning
./build/hyperion caption --image photo.jpg

# Visual question answering
./build/hyperion vqa --image photo.jpg --question "What color is the car?"
```

---

## 🎯 **Platform-Specific Quick Start**

### **WebAssembly (Browser)**

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh

# Build for WASM
cd platforms/wasm
cmake -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake ..
cmake --build .

# Test in browser
python3 -m http.server 8000
# Open: http://localhost:8000/hyperion_web_demo.html
```

### **Mobile (Android)**

```bash
# Setup Android NDK
export ANDROID_NDK=/path/to/android-ndk

# Build for Android
cd platforms/mobile/android
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake ..
cmake --build .
```

### **Embedded (Raspberry Pi)**

```bash
# Cross-compile for ARM
sudo apt-get install gcc-arm-linux-gnueabihf

# Build embedded version
cmake -DHYPERION_EMBEDDED=ON -DCMAKE_BUILD_TYPE=MinSizeRel ..
cmake --build .

# Deploy to Raspberry Pi
scp build/hyperion pi@raspberrypi.local:~/
```

---

## 🔧 **Development Setup**

### VS Code Integration

1. **Install extensions**:
   - C/C++ Extension Pack
   - CMake Tools
   - GitLens

2. **Configure workspace** (`.vscode/settings.json`):
```json
{
  "cmake.defaultVariants": {
    "buildType": "Release",
    "platform": "x64"
  },
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

### Qoder IDE Integration

The project is optimized for Qoder IDE:
- **Project root**: Auto-detected from `.qoder/` directory
- **Build integration**: Use Ctrl+Shift+B to build
- **Testing**: Use Ctrl+Shift+T to run tests
- **Debugging**: F5 to start debugging session

---

## ⚡ **Performance Tips**

### Memory Optimization
```bash
# Monitor memory usage
./build/tests/test_memory_analysis

# Enable memory profiling
export HYPERION_MEMORY_PROFILING=1
./build/hyperion generate --prompt "Test" --profile
```

### SIMD Acceleration
```bash
# Check SIMD support
./build/tests/test_simd_ops

# Benchmark SIMD performance
./build/tests/simd_benchmark
```

### Multi-threading
```bash
# Enable multi-threading
export OMP_NUM_THREADS=4
./build/hyperion generate --prompt "Test" --parallel
```

---

## 🔍 **Troubleshooting**

### Common Issues

#### **Build Fails**
```bash
# Clean build directory
rm -rf build/
mkdir build && cd build

# Reconfigure with verbose output
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build . --verbose
```

#### **Tests Fail**
```bash
# Run tests with verbose output
ctest -V

# Run specific failing test
./build/tests/test_memory --verbose
```

#### **Memory Issues**
```bash
# Run with memory debugging
export HYPERION_DEBUG_MEMORY=1
./build/hyperion generate --prompt "Test"

# Check for memory leaks (Linux/macOS)
valgrind --leak-check=full ./build/hyperion generate --prompt "Test"
```

#### **Performance Issues**
```bash
# Profile performance
./build/tests/performance_benchmark_suite --detailed

# Check system resources
top -p $(pgrep hyperion)
```

### Getting Help

- **Documentation**: See `README.md` for comprehensive guide
- **Examples**: Check `examples/` directory for detailed examples
- **Issues**: Open GitHub issue for bugs or feature requests
- **FAQ**: See `FAQ.md` for frequently asked questions

---

## 🎆 **What's Next?**

### Beginner Path
1. ✅ **Complete this quick start**
2. 📚 **Read `EXAMPLES.md`** - Comprehensive examples guide
3. 🛠️ **Try `examples/chatbot/`** - Build your first AI chatbot
4. 🎨 **Experiment with web interface** - Test real-time generation

### Intermediate Path
1. 📊 **Study `ARCHITECTURE.md`** - Understand system design
2. 🔧 **Read `DEVELOPMENT.md`** - Development guidelines
3. 🧪 **Try multimodal examples** - Image + text processing
4. 🚀 **Deploy to cloud** - Use `cloud/aws/` templates

### Advanced Path
1. 🧠 **Explore quantization** - Optimize model size
2. ⚙️ **Contribute to core** - See `CONTRIBUTING.md`
3. 📱 **Mobile development** - Build mobile apps
4. 🌍 **WebAssembly** - Deploy to browsers

---

**🚀 Ready to build amazing AI applications with Hyperion!**

*For detailed documentation, examples, and advanced topics, see the full documentation in the repository.*

---

*Quick Start Guide Version: 1.0*  
*Last Updated: 2025-08-23*  
*Estimated Setup Time: 5 minutes*