# 🔰 Beginner Examples

**Perfect for first-time Hyperion users** - These examples introduce core concepts with minimal complexity and clear explanations.

## 📚 Learning Path

Start with these examples in order to build your understanding of Hyperion:

### 1. Hello World Text Generation
**File**: `beginner_hello_world.c`  
**Memory**: ~16MB  
**Time**: 5 minutes

Your first Hyperion program! Demonstrates:
- Basic initialization
- Simple model creation
- Text generation with 10 tokens
- Proper cleanup

**What you'll learn:**
- How to initialize Hyperion
- Basic model and tokenizer concepts
- Simple text generation
- Memory management

**Run it:**
```bash
cd examples/
gcc -o beginner_hello_world beginner_hello_world.c \
    ../core/memory.c ../core/config.c \
    ../models/text/generate.c ../models/text/tokenizer.c \
    -I.. -lm
./beginner_hello_world
```

### 2. Configuration Basics (Coming Soon)
**File**: `beginner_config.c`  
**Memory**: ~8MB  
**Time**: 5 minutes

Learn how to configure Hyperion:
- Loading configuration files
- Setting parameters programmatically
- Understanding memory limits
- Performance tuning basics

### 3. File Loading (Coming Soon)
**File**: `beginner_file_loading.c`  
**Memory**: ~24MB  
**Time**: 10 minutes

Working with real model files:
- Loading pre-trained models
- Loading tokenizer vocabularies
- Error handling for file operations
- Model validation

### 4. Interactive Input (Coming Soon)
**File**: `beginner_interactive.c`  
**Memory**: ~32MB  
**Time**: 15 minutes

Creating interactive applications:
- Reading user input
- Streaming generation
- Simple conversation loop
- Basic context management

## 🎯 Quick Start

If you're completely new to Hyperion:

1. **Install Prerequisites:**
   ```bash
   # Make sure you have a C compiler
   gcc --version  # or clang --version
   ```

2. **Build Hyperion:**
   ```bash
   cd /path/to/hyperion
   mkdir build && cd build
   cmake .. -DBUILD_EXAMPLES=ON
   cmake --build .
   ```

3. **Run Your First Example:**
   ```bash
   cd examples/
   ./beginner_hello_world
   ```

4. **Expected Output:**
   ```
   === Hyperion Hello World Example ===
   This example demonstrates basic text generation.
   
   Step 1: Initializing Hyperion...
   ✓ Hyperion initialized successfully
   
   Step 2: Setting up configuration...
   ✓ Configuration set for minimal memory usage
   
   Step 3: Creating tokenizer...
   ✓ Simple tokenizer created with 29 tokens
   
   Step 4: Creating minimal text generation model...
   ✓ Minimal model created (Hidden: 64, Layers: 2, Vocab: 29)
   
   Step 5: Generating text...
   Prompt: "Hello"
   Generated text: hello world the good nice
   
   Step 6: Memory usage summary...
   ✓ Current memory usage: 8.45 MB
   ✓ Memory tracking: 156 allocations
   
   === Example Completed Successfully! ===
   ```

## 🔧 Common Issues

### Build Errors
```bash
# If you get "file not found" errors:
export C_INCLUDE_PATH=/path/to/hyperion:$C_INCLUDE_PATH

# If linking fails:
export LIBRARY_PATH=/path/to/hyperion/build:$LIBRARY_PATH
```

### Runtime Errors
```bash
# If you get memory allocation errors:
ulimit -v 1000000  # Increase virtual memory limit

# If models fail to load:
ls -la models/  # Check if model files exist
```

### Performance Issues
```bash
# If generation is very slow:
# 1. Check available memory
free -h

# 2. Use smaller model parameters
# Edit the example and reduce hiddenSize, numLayers
```

## 📖 Understanding the Code

### Key Concepts

**Tokenizer**: Converts text to numbers and back
```c
HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
hyperionAddTokenToVocabulary(tokenizer, "hello", 0);
```

**Model**: The AI that generates text
```c
HyperionModel *model = hyperionCreateModel();
HyperionModelConfig config = {.vocabSize = 100, .hiddenSize = 64};
hyperionInitializeModel(model, &config);
```

**Generation**: Creating new text
```c
HyperionGenerationParams params = {.maxTokens = 10, .temperature = 0.7f};
hyperionGenerateText(model, &params, outputTokens, 32);
```

**Memory Management**: Tracking allocations
```c
hyperionMemTrackInit();        // Start tracking
// ... use Hyperion functions ...
hyperionMemTrackDumpLeaks();   // Check for leaks
hyperionMemTrackCleanup();     // Stop tracking
```

### Code Structure Pattern

All beginner examples follow this pattern:
1. **Initialize** - Set up Hyperion subsystems
2. **Configure** - Set parameters for your use case
3. **Create** - Make tokenizer and model objects
4. **Generate** - Produce AI output
5. **Display** - Show results to user
6. **Cleanup** - Free all resources

## 🚀 Next Steps

Once you're comfortable with these beginner examples:

1. **Explore Intermediate Examples** - More realistic applications
   - `examples/intermediate/` - Memory optimization, file I/O, error handling

2. **Try Advanced Examples** - Production-ready features
   - `examples/advanced/` - Multimodal, streaming, performance tuning

3. **Build Your Own Application** - Use Hyperion in your project
   - Check `EXAMPLES.md` for comprehensive examples guide
   - See `ARCHITECTURE.md` for technical details

4. **Join the Community** - Get help and share your creations
   - GitHub Issues for bug reports
   - GitHub Discussions for questions
   - Contributing guide in `CONTRIBUTING.md`

## 💡 Tips for Success

1. **Start Small** - Don't try to build complex applications immediately
2. **Read the Output** - Pay attention to memory usage and error messages
3. **Experiment** - Modify the examples to understand how they work
4. **Ask Questions** - Use GitHub Discussions if you get stuck
5. **Check Examples** - Look at `examples/chatbot/` for a complete application

Remember: Hyperion is designed to be lightweight and efficient. These beginner examples use minimal resources so you can focus on learning the concepts rather than dealing with complex setups.

**Happy coding with Hyperion! 🎉**