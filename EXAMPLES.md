# Hyperion AI Framework - Examples Guide

*Comprehensive examples from beginner to advanced level*

---

## 🎆 **Quick Start Examples**

### Hello World (30 seconds)

```bash
# Build and run
./build/hyperion generate --prompt "Hello, AI world!" --max-tokens 20

# Expected output:
# "Hello, AI world! I'm Hyperion, ready to help with your tasks."
```

### Web Interface Demo (2 minutes)

```bash
# Start web server
./build_webserver.bat  # Windows
./build/interface/web_server  # Linux/macOS

# Open browser: http://localhost:8080
# Try the interactive demo!
```

---

## 📚 **Beginner Examples**

### 1. Text Generation CLI

```bash
# Simple text completion
./build/hyperion generate \
  --prompt "The future of AI is" \
  --max-tokens 50 \
  --temperature 0.7

# Batch processing
echo "Tell me about space" | ./build/hyperion generate --stream
```

### 2. Configuration Files

**config.json**:
```json
{
  "model": {
    "type": "text",
    "quantization": "4bit",
    "max_context_length": 1024
  },
  "generation": {
    "temperature": 0.8,
    "top_k": 50,
    "max_tokens": 100
  }
}
```

### 3. Simple C Integration

```c
#include "hyperion.h"

int main() {
    // Initialize
    hyperion_config_t config = hyperion_default_config();
    hyperion_context_t* ctx = hyperion_init(&config);
    
    // Generate
    const char* result = hyperion_generate_text(ctx, "Hello");
    printf("AI: %s\n", result);
    
    hyperion_cleanup(ctx);
    return 0;
}
```

---

## 🛠️ **Intermediate Examples**

### 1. Chatbot Application

**Location**: `examples/chatbot/`

```bash
cd examples/chatbot
cmake --build . && ./chatbot
```

**Features**:
- Interactive conversation
- Context preservation
- Custom personalities
- Memory-efficient operation

### 2. Image Classification

**Location**: `examples/image_recognition/`

```bash
# Classify single image
./build/hyperion classify --image photo.jpg

# Batch classification
./build/hyperion classify --batch images/ --output results.json
```

### 3. Audio Processing

```bash
# Speech recognition
./build/hyperion transcribe --audio recording.wav

# Voice activity detection
./build/hyperion detect-voice --audio stream.wav --threshold 0.5

# Keyword spotting
./build/hyperion spot-keywords --audio input.wav --keywords "hello,help"
```

### 4. Web API Server

```bash
# Start API server
./build/interface/web_server --port 8080 --api-only

# Test endpoints
curl -X POST http://localhost:8080/api/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Explain AI", "max_tokens": 100}'
```

---

## 🚀 **Advanced Examples**

### 1. Multimodal Processing

**Image Captioning**:
```bash
./build/hyperion caption --image sunset.jpg
# Output: "A beautiful sunset over mountains with golden clouds"
```

**Visual Question Answering**:
```bash
./build/hyperion vqa \
  --image car.jpg \
  --question "What color is the car?"
# Output: "The car is red"
```

### 2. Real-time Streaming

**WebSocket Client** (JavaScript):
```javascript
const ws = new WebSocket('ws://localhost:8080/websocket');

ws.onopen = () => {
    ws.send(JSON.stringify({
        action: 'generate',
        prompt: 'Tell me a story',
        stream: true
    }));
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Token:', data.token);
};
```

### 3. Custom Model Integration

```c
// Load custom quantized model
hyperion_model_t* model = hyperion_load_custom_model(
    ctx, 
    "custom_model.bin", 
    QUANTIZATION_4BIT
);

// Configure for specific use case
hyperion_model_config_t config = {
    .context_length = 2048,
    .batch_size = 1,
    .enable_sparse = true,
    .memory_pool_size = 128 * 1024 * 1024
};

hyperion_configure_model(model, &config);
```

### 4. Performance Optimization

```c
// Enable SIMD acceleration
hyperion_enable_simd(ctx, SIMD_AVX2);

// Configure memory pools
hyperion_memory_config_t mem_config = {
    .pool_count = 4,
    .pool_sizes = {1024, 4096, 16384, 65536},
    .enable_profiling = true
};

hyperion_configure_memory(ctx, &mem_config);

// Multi-threading
hyperion_set_thread_count(ctx, 4);
```

---

## 🌐 **Platform-Specific Examples**

### WebAssembly (Browser)

**HTML Integration**:
```html
<!DOCTYPE html>
<html>
<head>
    <script src="hyperion.js"></script>
</head>
<body>
    <textarea id="prompt" placeholder="Enter prompt..."></textarea>
    <button onclick="generate()">Generate</button>
    <div id="result"></div>
    
    <script>
        let hyperion;
        
        Hyperion().then(instance => {
            hyperion = instance;
        });
        
        async function generate() {
            const prompt = document.getElementById('prompt').value;
            const result = hyperion.generateText(prompt);
            document.getElementById('result').innerText = result;
        }
    </script>
</body>
</html>
```

### Mobile (Android)

**JNI Integration**:
```java
public class HyperionAI {
    static {
        System.loadLibrary("hyperion");
    }
    
    public native String generateText(String prompt);
    public native void initialize(String configPath);
    public native void cleanup();
}
```

### Embedded (Arduino)

```cpp
#include "hyperion_embedded.h"

void setup() {
    Serial.begin(115200);
    
    // Initialize with minimal config
    hyperion_embedded_config_t config = {
        .memory_limit = 32 * 1024,  // 32KB
        .model_type = HYPERION_TEXT_TINY,
        .quantization = QUANTIZATION_4BIT
    };
    
    hyperion_embedded_init(&config);
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readString();
        String output = hyperion_embedded_generate(input.c_str());
        Serial.println(output);
    }
}
```

---

## 📊 **Performance Examples**

### Memory Profiling

```bash
# Enable memory profiling
export HYPERION_MEMORY_PROFILING=1
./build/hyperion generate --prompt "Test" --profile

# Detailed memory analysis
./build/tests/test_memory_analysis --detailed
```

### Benchmarking

```bash
# Performance benchmarks
./build/tests/performance_benchmark_suite

# SIMD performance
./build/tests/simd_benchmark

# Memory efficiency test
./build/tests/test_memory_efficient_tensor
```

### Load Testing

```bash
# Concurrent requests
for i in {1..100}; do
    curl -X POST http://localhost:8080/api/generate \
        -H "Content-Type: application/json" \
        -d '{"prompt": "Test '$i'", "max_tokens": 10}' &
done
wait
```

---

## 🔧 **Development Examples**

### Custom Commands

```c
// Add custom CLI command
int custom_analyze_command(int argc, char* argv[]) {
    // Custom analysis logic
    return 0;
}

// Register command
hyperion_register_command("analyze", custom_analyze_command);
```

### Plugin System

```c
// Custom plugin
typedef struct {
    hyperion_plugin_t base;
    // Custom data
} custom_plugin_t;

int custom_plugin_init(hyperion_plugin_t* plugin) {
    // Initialize plugin
    return HYPERION_SUCCESS;
}

// Register plugin
hyperion_register_plugin("custom", custom_plugin_init);
```

---

## 📚 **Complete Working Examples**

### Example Directory Structure

```
examples/
├── BEGINNER.md                    # Beginner guide
├── EXAMPLES_INDEX.md              # This file
├── beginner_hello_world.c         # Hello world
├── intermediate_file_processor.c  # File processing
├── advanced_streaming_server.c    # Streaming server
├── chatbot/                       # Interactive chatbot
├── document_processor/            # Document analysis
├── image_recognition/             # Image classification
├── media_tagging/                 # Media analysis
├── multimodal/                    # Multimodal processing
└── complete_demo/                 # Full feature demo
```

### Running Examples

```bash
# Build all examples
cd examples
cmake --build .

# Run specific example
./chatbot/chatbot
./image_recognition/image_classifier --image test.jpg
./multimodal/image_captioning/caption --image photo.jpg
```

---

## 🎆 **Next Steps**

### Learning Path
1. **Start with**: Hello World example
2. **Practice**: Chatbot and web interface
3. **Explore**: Multimodal capabilities
4. **Advanced**: Custom integrations
5. **Contribute**: Add your own examples

### Resources
- **Quick Start**: `QUICK_START.md`
- **FAQ**: `FAQ.md`
- **Architecture**: `ARCHITECTURE.md`
- **Development**: `DEVELOPMENT.md`

---

*Examples Guide Version: 1.0*  
*Last Updated: 2025-08-23*  
*All examples tested with Hyperion v0.1.0*