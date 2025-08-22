# Getting Started with Hyperion

This guide will help you get started with the Hyperion framework. You'll learn how to install Hyperion, run a simple example, and understand the basic concepts.

## Installation

### Prerequisites

Before installing Hyperion, ensure you have the following prerequisites:

- C compiler (GCC, Clang, or MSVC)
- CMake (version 3.10 or later)
- Git (for cloning the repository)

For optimal performance, your hardware should support SIMD instructions:
- SSE2 (minimum requirement)
- AVX/AVX2 (recommended for better performance)

### Installation Steps

#### 1. Clone the Repository

```bash
git clone https://github.com/TheLakeMan/hyperion.git
cd hyperion
```

#### 2. Build Hyperion

```bash
# Create and enter build directory
mkdir build && cd build

# Configure (choose appropriate generator)
# Windows:
cmake -G "Visual Studio 17 2022" ..
# Linux/macOS:
cmake ..

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --prefix /path/to/install
```

#### 3. Verify Installation

Run the included test suite to verify that Hyperion is working correctly:

```bash
ctest -C Release
```

All tests should pass. If any tests fail, please check the troubleshooting section or report the issue.

## Quick Start Example

Here's a simple example to get you started with text generation:

```c
#include <hyperion/core.h>
#include <hyperion/models/text.h>
#include <stdio.h>

int main() {
    // Initialize Hyperion
    hyperion_init();
    
    // Load a small text model (included with Hyperion)
    HyperionModel* model = hyperion_load_model("models/small_text_model.tmai");
    if (!model) {
        printf("Failed to load model\n");
        return 1;
    }
    
    // Generate text
    const char* prompt = "Hyperion is";
    char output[256];
    
    hyperion_generate_text(model, prompt, output, sizeof(output), 50);
    
    // Print the generated text
    printf("Generated: %s\n", output);
    
    // Clean up
    hyperion_free_model(model);
    hyperion_shutdown();
    
    return 0;
}
```

Compile and run this example with:

```bash
gcc -o text_example text_example.c -lhyperion
./text_example
```

## Basic Concepts

Hyperion is designed around several key concepts:

### 1. Models

Models are the core of Hyperion. They represent the neural networks that perform inference. Models are loaded from `.tmai` files, which contain the quantized weights and model structure.

```c
// Loading a model
HyperionModel* model = hyperion_load_model("path/to/model.tmai");

// Using memory-mapped loading for larger models
HyperionMmapConfig config = hyperionCreateDefaultMmapConfig();
HyperionMappedModel* mappedModel = hyperionOpenMappedModel("path/to/large_model.tmai", &config);
```

### 2. Modalities

Hyperion supports different modalities:

- **Text**: Text generation, completion, and classification
- **Image**: Image classification, detection, and processing
- **Audio**: Keyword spotting, voice activity detection, speech recognition
- **Multimodal**: Combining different modalities, such as image captioning

Each modality has its own API and utilities tailored to its specific needs.

### 3. Quantization

Hyperion uses 4-bit quantization by default to reduce model size and improve performance. This allows models to run on devices with limited memory.

Advanced quantization techniques include:
- Mixed precision quantization
- Model pruning
- Quantization-aware training

### 4. Memory Optimization

For larger models, Hyperion provides memory optimization through:

```c
// Memory-mapped model loading
HyperionMappedModel* model = hyperionOpenMappedModel("path/to/model.tmai", NULL);

// Forward pass scheduling
HyperionForwardScheduler* scheduler = hyperionCreateForwardScheduler(
    model, HYPERION_EXEC_MEMORY_OPT, 100 * 1024 * 1024); // 100MB limit
```

### 5. Performance Optimization

Hyperion leverages SIMD instructions for optimal performance:

```c
// Check SIMD support
if (hyperion_has_avx2_support()) {
    printf("AVX2 support detected! Optimal performance available.\n");
} else if (hyperion_has_sse2_support()) {
    printf("SSE2 support detected. Good performance available.\n");
} else {
    printf("SIMD not supported. Performance will be limited.\n");
}
```

## Next Steps

After getting started, you might want to explore:

- [Basic Usage Guide](basic_usage.md) - More comprehensive usage examples
- [Text Generation Guide](text_generation.md) - In-depth guide for text generation
- [Memory Optimization Guide](memory_optimization.md) - How to optimize memory usage
- [Example Applications](../examples/chatbot.md) - Complete example applications

## Troubleshooting

### Common Issues

1. **Compilation errors**: Ensure you have the required dependencies and a compatible compiler.
2. **Model loading errors**: Check that the model file exists and is a valid `.tmai` file.
3. **Performance issues**: Verify that SIMD instructions are enabled and supported by your hardware.

For more help, check the [Debugging Guide](../dev/debugging.md) or submit an issue on the GitHub repository.