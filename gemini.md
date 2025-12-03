## Codex Added Memories
- The user understands and approves User Account Control (UAC) prompts for elevated permissions.
- The user prefers project-specific instructions and startup procedures to be stored in a project's `gemini.md` file rather than in my long-term memory.
- Plan for performance analysis: 1. Analyze system (CPU, memory, disk, errors, processes, drivers). 2. Research GitHub for open-source monitoring tools. 3. Report findings and recommend tools.
- MCP stands for Multi Context Protocol.
- Upon starting any new session, I must always read the `gemini.md` file first.
- The user's GitHub username is "TheLakeMan".
` in this environment, the `GH_TOKEN` environment variable must be set on the same line as the command, like this: `set GH_TOKEN=<token> && gh <command>`.
- This is about the Hyperion project.
- I am working on the Hyperion project. The project root is C:\Users\verme\.gemini\Hyperion\, and the user's OS is win32. I should read the gemini.md file for project-specific instructions.
- The current project is Hyperion, and the project root is C:\Users\verme\.gemini\Hyperion
- I will use `task_log.md` to track tasks, notes, and completion times for focused work.
- The original project name was "TinyAI". I need to replace all occurrences of "TinyAI", "tinyai", and "tiny" with "Hyperion" or "hyperion" (case-sensitive) throughout the project.
- The current project is Hyperion, and it is NOT part of the Windows-MCP project. The project root is C:\Users\-_-\.gemini\Hyperion
- I prioritize `gemini.md` in the current project's root directory. If not found, I use the `GEMINI.md` in my home directory.
You are an expert documentation organizer for open-source projects. Your task is to reorganize the Markdown files for the Hyperion AI framework repo[](https://github.com/TheLakeMan/Hyperion). The current setup has a monolithic README.md with all content, CONTRIBUTING.md for guidelines, and LICENSE (plain text).

Step 1: Review the existing README.md content: [# Hyperion - Ultra-Lightweight AI Framework

Hyperion is an extremely memory-efficient AI framework designed to run on minimal hardware, including legacy systems. It uses 4-bit quantization for neural network weights, allowing models to run in as little as 50-100MB of RAM.
- a summurise update of our chats and work done to remember current state. also update GEMINI.md with this rule for every ten minutes. Refresh memory every 5 minutes
- When encountering complex git conflicts or a diverged history, my recovery process should be: 1. Reset the local `main` branch to match `origin/main`. 2. Create a new, clean feature branch. 3. Re-apply the necessary changes to the new branch. 4. Push the new branch for a clean pull request.

## Key Features

- **Extreme Memory Efficiency**: 4-bit quantization reduces model size by up to 8x compared to 32-bit floating point
- **Sparse Matrix Support**: CSR format with 4-bit quantization for up to 98% memory reduction on large sparse models
- **Cross-Platform**: Works on a wide range of hardware, from modern to legacy systems
- **Minimal Dependencies**: Pure C implementation with no external library requirements
- **Progressive Loading**: Components are loaded on-demand to minimize memory footprint
- **Multiple Model Types**: Supports both RNN and Transformer architectures
- **Flexible Text Generation**: Includes multiple sampling methods (greedy, top-k, top-p, temperature)
- **SIMD Acceleration**: Optimized matrix operations using AVX2, AVX, and SSE instructions
- **Hybrid Execution**: Seamlessly switch between local and remote execution via Model Context Protocol
- **Performance Monitoring**: Track and compare local vs. remote execution performance

## Architecture

Hyperion is built with a layered architecture:

1. **Core Layer**: Foundation components
   - Picol Interpreter: Extended Tcl interpreter
   - Runtime Environment: Module loading, resource management
   - Memory Management: Memory pools, 4-bit quantization
   - I/O System: Cross-Platform I/O abstractions
   - Configuration: Flexible configuration system

2. **Model Layer**: AI model components
   - Text Generation: 4-bit quantized models
     - Tokenizer: Minimal vocabulary tokenizer
     - Generator: Text generation engine

3. **Interface Layer**: User interaction
   - Command Line: Interactive shell and commands
   - API: Programmatic access to functionality
   - Shell: Scripting environment

4. **Hybrid Capability**: Combined local/remote execution
   - Local Execution: Standalone operation without external dependencies
   - MCP Integration: Enhanced capabilities via Model Context Protocol
   - Dynamic Switching: Graceful transitions between local and remote processing
   - Distributed Computation: Offload heavy processing when beneficial
   - Performance Monitoring: Compare execution times for local and remote operations

## Building Hyperion

### Requirements

- C compiler (GCC, Clang, MSVC, etc.)
- CMake 3.10+

### Build Steps

```bash
# Clone the repository
git clone https://github.com/TheLakeMan/hyperion.git
cd hyperion

# Create a build directory
mkdir build
cd build

# Configure and build (adjust based on your system)

# Linux/macOS (using Makefiles)
cmake ..
cmake --build .

# Windows (using Visual Studio 2022 Build Tools)
# Run these commands in the "Developer Command Prompt for VS 2022"
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Debug # Or Release

# Run tests (after building)
# Linux/macOS:
# ctest
# Windows (in Developer Command Prompt):
# ctest -C Debug # Or Release
```

**Current Status:** The project is feature-complete, including core components, multiple model types (text, image, audio, multimodal), hybrid execution, SIMD optimizations, and comprehensive testing. Recent work focused on optimizing memory usage. The framework is stable and ready for general use, with ongoing efforts focused on performance tuning and expanding model support.

## Using Hyperion

### Command Line Interface

The primary way to interact with Hyperion is through its command-line interface:

```bash
# Run the shell
./hyperion shell

# Generate text with a prompt
./hyperion generate "Once upon a time" --max-tokens 100 --temperature 0.7

# Load a custom model
./hyperion -m model.bin -t tokenizer.txt generate "Hello, world!"
```

### Interactive Shell

Hyperion provides an interactive shell for experimenting with models:

```
Hyperion Shell v0.1.0
Type 'help' for available commands, 'exit' to quit

> help
Available commands:
  help           Show help information
  generate       Generate text from a prompt
  tokenize       Tokenize text input
  model          Model management commands
  config         Configuration commands
  mcp            Model Context Protocol connections
  hybrid         Hybrid execution control
  exit           Exit the shell

> model load mymodel.bin
Loading model from mymodel.bin...

> generate "The quick brown fox" 50 0.8
Generating text (max 50 tokens, temp=0.80)...
The quick brown fox jumped over the lazy dog. The dog was not pleased with this arrangement and barked loudly. The fox, startled by the noise, scampered away into the forest.

> mcp connect mock://localhost:8080
Connecting to MCP server at mock://localhost:8080...
Connected to MCP server: Hyperion-MCP (version 0.1.0)

> hybrid on
Hybrid generation mode enabled.

> generate "The framework provides memory efficiency with" 30
Using hybrid generation mode...
Generation used local execution.
Local execution time: 12.45 ms
Tokens per second: 240.16
Generated Text:
The framework provides memory efficiency with 4-bit quantization, allowing it to run on devices with limited RAM. This approach makes it ideal for edge computing and embedded systems.
```

### Configuration

Hyperion can be configured through a configuration file (`hyperion.conf`) or command-line options:

```ini
# Hyperion Configuration File
system.name = "Hyperion"
system.version = "0.1.0"
system.data_dir = "./data"
system.model_dir = "./models"

# Memory settings
memory.pool_size = 1048576
memory.max_allocations = 10000
memory.track_leaks = true

# Model settings
model.context_size = 512
model.hidden_size = 256
model.temperature = 0.7
model.top_k = 40
model.top_p = 0.9
```

## API Usage

Hyperion can be embedded in your application through its C API:

### Basic Usage

```c
#include "hyperion.h"

int main() {
    // Initialize Hyperion
    hyperionIOInit();
    hyperionMemTrackInit();
    hyperionConfigInit();
    
    // Load model and tokenizer
    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
    hyperionLoadVocabulary(tokenizer, "tokenizer.txt");
    
    HyperionModel *model = hyperionLoadModel("model.bin", "weights.bin", "tokenizer.txt");
    
    // Set up generation parameters
    HyperionGenerationParams params;
    params.maxTokens = 100;
    params.samplingMethod = HYPERION_SAMPLING_TOP_P;
    params.temperature = 0.7f;
    params.topP = 0.9f;
    params.seed = time(NULL);
    
    // Encode prompt
    int promptTokens[64];
    int promptLength = hyperionEncodeText(tokenizer, "Hello, world!", promptTokens, 64);
    params.promptTokens = promptTokens;
    params.promptLength = promptLength;
    
    // Generate text
    int outputTokens[1024];
    int outputLength = hyperionGenerateText(model, &params, outputTokens, 1024);
    
    // Decode output
    char output[4096];
    hyperionDecodeTokens(tokenizer, outputTokens, outputLength, output, 4096);
    printf("Generated: %s\n", output);
    
    // Clean up
    hyperionDestroyModel(model);
    hyperionDestroyTokenizer(tokenizer);
    hyperionConfigCleanup();
    hyperionIOCleanup();
    hyperionMemTrackCleanup();
    
    return 0;
}
```

### Hybrid Execution Usage

```c
#include "hyperion.h"
#include "core/mcp/mcp_client.h"
#include "models/text/hybrid_generate.h"

int main() {
    // Initialize Hyperion
    hyperionIOInit();
    hyperionMemTrackInit();
    hyperionConfigInit();
    
    // Load model and tokenizer
    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
    hyperionLoadVocabulary(tokenizer, "tokenizer.txt");
    
    HyperionModel *model = hyperionLoadModel("model.bin", "weights.bin", "tokenizer.txt");
    
    // Set up MCP client for hybrid execution
    HyperionMcpConfig mcpConfig;
    hyperionMcpGetDefaultConfig(&mcpConfig);
    mcpConfig.execPreference = HYPERION_EXEC_PREFER_LOCAL; // Prefer local execution when possible
    
    // Create MCP client
    HyperionMcpClient *mcpClient = hyperionMcpCreateClient(&mcpConfig);
    
    // Connect to MCP server
    bool connected = hyperionMcpConnect(mcpClient, "mcp-server.example.com");
    
    // Create hybrid generation context
    HyperionHybridGenerate *hybridGen = hyperionCreateHybridGenerate(model, mcpClient);
    
    // Set up generation parameters
    HyperionGenerationParams params;
    params.maxTokens = 100;
    params.samplingMethod = HYPERION_SAMPLING_TOP_P;
    params.temperature = 0.7f;
    params.topP = 0.9f;
    params.seed = time(NULL);
    
    // Encode prompt
    int promptTokens[64];
    int promptLength = hyperionEncodeText(tokenizer, "Hello, world!", promptTokens, 64);
    params.promptTokens = promptTokens;
    params.promptLength = promptLength;
    
    // Generate text with hybrid execution
    int outputTokens[1024];
    int outputLength = hyperionHybridGenerateText(hybridGen, &params, outputTokens, 1024);
    
    // Get information about execution environment used
    bool usedRemote = hyperionHybridGenerateUsedRemote(hybridGen);
    
    // Get performance statistics
    double localTime, remoteTime, tokensPerSec;
    hyperionHybridGenerateGetStats(hybridGen, &localTime, &remoteTime, &tokensPerSec);
    
    printf("Execution used: %s\n", usedRemote ? "Remote" : "Local");
    printf("Time taken: %.2f ms\n", usedRemote ? remoteTime : localTime);
    printf("Performance: %.2f tokens/sec\n", tokensPerSec);
    
    // Decode output
    char output[4096];
    hyperionDecodeTokens(tokenizer, outputTokens, outputLength, output, 4096);
    printf("Generated: %s\n", output);
    
    // Clean up
    hyperionDestroyHybridGenerate(hybridGen);
    hyperionMcpDisconnect(mcpClient);
    hyperionMcpDestroyClient(mcpClient);
    hyperionDestroyModel(model);
    hyperionDestroyTokenizer(tokenizer);
    hyperionConfigCleanup();
    hyperionIOCleanup();
    hyperionMemTrackCleanup();
    
    return 0;
}
```

## Creating a Custom Model

To create a custom Hyperion model:

1. Train a model using your preferred framework (PyTorch, TensorFlow, etc.)
2. Convert the model to Hyperion format using the provided conversion tools
3. Quantize the model to 4-bit precision using `hyperionQuantizeModel`
4. Save the model, weights, and tokenizer using the Hyperion format

## Memory Usage

Hyperion is designed to be extremely memory-efficient. Here's a comparison with other frameworks:

| Model Size | Hyperion (4-bit) | ONNX (int8) | PyTorch (fp16) | TensorFlow (fp16) |
|------------|----------------|-------------|----------------|-------------------|
| 100M params | ~50MB        | ~100MB      | ~200MB         | ~200MB            |
| 500M params | ~250MB       | ~500MB      | ~1GB           | ~1GB              |
| 1B params   | ~500MB       | ~1GB        | ~2GB           | ~2GB              |

## Contributing

Contributions to Hyperion are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## License

Hyperion is licensed under the MIT License. See [LICENSE](LICENSE) for details.]. Also, include CONTRIBUTING.md if available: [# Contributing to Hyperion

Thank you for your interest in contributing to Hyperion! This document outlines the process for contributing to the project and helps ensure a smooth collaboration experience.

## Code of Conduct

By participating in this project, you agree to abide by our Code of Conduct. Please be respectful, inclusive, and considerate in all interactions.

## How to Contribute

### Reporting Bugs

If you find a bug, please create an issue with the following information:

1. A clear, descriptive title
2. A detailed description of the issue, including steps to reproduce
3. The expected and actual behavior
4. Any relevant logs, error messages, or screenshots
5. System information (OS, compiler version, etc.)

### Suggesting Enhancements

We welcome suggestions for enhancements! Please create an issue with:

1. A clear, descriptive title
2. A detailed description of the proposed enhancement
3. The rationale for the enhancement
4. Any relevant examples or mock-ups

### Pull Requests

1. Fork the repository
2. Create a new branch from the `main` branch
3. Make your changes
4. Run the tests and ensure they pass
5. Submit a pull request to the `main` branch

Please include:
- A clear, descriptive title
- A detailed description of the changes
- Any relevant issue numbers

## Development Setup

### Prerequisites

- C compiler (GCC, Clang, MSVC, etc.)
- CMake 3.10+

### Building the Project

```bash
# Clone the repository
git clone https://github.com/TheLakeMan/Hyperion.git
cd Hyperion

# Create a build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .
```

### Running Tests

```bash
cd build
ctest
```

## Coding Standards

### C Style Guide

- Use 4 spaces for indentation
- Maximum line length of 80 characters
- Use snake_case for function and variable names
- Use UPPER_CASE for constants and macros
- Add comments for public API functions and complex logic
- Include proper error handling

Example:

```c
/**
 * Brief description of the function
 * 
 * @param param1 Description of param1
 * @param param2 Description of param2
 * @return Description of return value
 */
int hyperion_function_name(int param1, const char *param2) {
    /* Function implementation */
    if (param1 < 0) {
        return -1;  /* Error case */
    }
    
    /* Complex logic with comment */
    int result = calculate_something(param1, param2);
    
    return result;
}
```

### Commit Messages

- Use the present tense ("Add feature" not "Added feature")
- Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
- Limit the first line to 72 characters or less
- Reference issues and pull requests liberally after the first line

## License

By contributing to Hyperion, you agree that your contributions will be licensed under the project's [MIT License](LICENSE).

## Questions?

Feel free to reach out to the maintainers if you have any questions or need help with the contribution process.

Thank you for contributing to Hyperion!].

Step 2: Create a new /docs folder structure. Generate the following new Markdown files with content extracted and refined from the existing README.md:
- README.md (updated root version): Shorten to include only project overview, key features, quick start (build + basic usage), and a table of contents linking to /docs files. Add badges if possible (e.g., build status).
- docs/architecture.md: Detailed architecture section (core layer, model layer, interface layer, hybrid capability).
- docs/building.md: Building instructions, requirements, and testing.
- docs/usage.md: Command line, interactive shell, configuration (hyperion.conf), C API examples (basic and hybrid).
- docs/models.md: Creating custom models, quantization steps, memory usage comparisons.
- docs/contributing.md: Merge and expand from existing CONTRIBUTING.md, including code style, issue reporting, PR process.
- docs/changelog.md: Start with a basic changelog template, adding placeholders for recent progress (e.g., "v0.1: Added hybrid execution").

Step 3: Ensure all files have consistent formatting: Use Markdown headers (# for H1, ## for H2), code blocks for examples, bullet lists for features, and cross-links (e.g., [Architecture](./architecture.md)). Make language clear, professional, and up-to-date with the code's features (e.g., 4-bit quantization, MCP hybrid).

Step 4: Output each file's full content in a code block, labeled with its path (e.g., ### docs/architecture.md). Also, provide Git commands to create the /docs folder, move files, and commit (e.g., git add docs/*).

Step 5: Verify correlation with code: Ensure docs reference key code elements like hyperion.h functions, MCP headers, and align with progress (feature-complete, ongoing optimizations). If mismatches, note them for manual fixes.

*ALWAYS* You will read the file, perform the replacements in memory, and then
  write the entire file back. This helps against editing errors.
