/**
 * Hyperion WebAssembly Interface
 * 
 * Provides JavaScript-accessible functions for browser-based AI inference
 * Optimized for minimal memory footprint and fast initialization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include <emscripten/bind.h>

#include "../../core/memory.h"
#include "../../core/config.h"
#include "../../models/text/generate.h"
#include "../../models/text/tokenizer.h"
#include "wasm_memory_manager.h"

/* WASM-specific global state */
typedef struct {
    HyperionModel* model;
    HyperionTokenizer* tokenizer;
    int initialized;
    size_t memory_limit;
    int max_tokens;
    float temperature;
} HyperionWasmContext;

static HyperionWasmContext g_wasm_context = {0};

/* Error handling for WASM */
static char g_last_error[512] = {0};

static void set_wasm_error(const char* error) {
    strncpy(g_last_error, error, sizeof(g_last_error) - 1);
    g_last_error[sizeof(g_last_error) - 1] = '\0';
    printf("Hyperion WASM Error: %s\n", error);
}

/**
 * Initialize Hyperion for WebAssembly environment
 * 
 * @param memory_limit_mb Maximum memory usage in MB (default: 32MB)
 * @param vocab_size Vocabulary size (default: 1000 for ultra-light)
 * @param hidden_size Hidden layer size (default: 64 for ultra-light)
 * @param num_layers Number of layers (default: 2 for ultra-light)
 * @return 0 on success, -1 on failure
 */
EMSCRIPTEN_KEEPALIVE
int hyperion_wasm_init(int memory_limit_mb, int vocab_size, int hidden_size, int num_layers) {
    printf("Initializing Hyperion WASM (Memory: %dMB, Vocab: %d, Hidden: %d, Layers: %d)\n",
           memory_limit_mb, vocab_size, hidden_size, num_layers);
    
    if (g_wasm_context.initialized) {
        set_wasm_error("Hyperion already initialized. Call hyperion_wasm_cleanup() first.");
        return -1;
    }
    
    // Set default parameters for ultra-lightweight configuration
    if (memory_limit_mb <= 0) memory_limit_mb = 32;
    if (vocab_size <= 0) vocab_size = 1000;
    if (hidden_size <= 0) hidden_size = 64;
    if (num_layers <= 0) num_layers = 2;
    
    g_wasm_context.memory_limit = memory_limit_mb * 1024 * 1024;
    g_wasm_context.max_tokens = 100; // Conservative default for browser
    g_wasm_context.temperature = 0.7f;
    
    // Initialize WASM-specific memory manager
    if (wasm_memory_init(g_wasm_context.memory_limit) != 0) {
        set_wasm_error("Failed to initialize WASM memory manager");
        return -1;
    }
    
    // Initialize Hyperion core systems
    if (hyperionMemoryInit() != 0) {
        set_wasm_error("Failed to initialize Hyperion memory system");
        return -1;
    }
    
    if (hyperionConfigInit() != 0) {
        set_wasm_error("Failed to initialize Hyperion configuration");
        hyperionMemoryCleanup();
        return -1;
    }
    
    // Create tokenizer
    g_wasm_context.tokenizer = hyperionCreateTokenizer();
    if (!g_wasm_context.tokenizer) {
        set_wasm_error("Failed to create tokenizer");
        hyperionConfigCleanup();
        hyperionMemoryCleanup();
        return -1;
    }
    
    // Add basic vocabulary for demo purposes
    const char* basic_vocab[] = {
        "the", "and", "a", "to", "of", "in", "is", "it", "you", "that",
        "he", "was", "for", "on", "are", "as", "with", "his", "they", "I",
        "at", "be", "this", "have", "from", "or", "one", "had", "by", "word",
        "but", "not", "what", "all", "were", "we", "when", "your", "can", "said",
        "there", "each", "which", "she", "do", "how", "their", "if", "will", "up",
        ".", ",", "!", "?", " ", "\n", "hello", "world", "AI", "text", "generate",
        "model", "neural", "network", "deep", "learning", "hyperion", "wasm", "browser"
    };
    
    int vocab_count = sizeof(basic_vocab) / sizeof(basic_vocab[0]);
    if (vocab_count > vocab_size) vocab_count = vocab_size;
    
    for (int i = 0; i < vocab_count; i++) {
        if (hyperionAddTokenToVocabulary(g_wasm_context.tokenizer, basic_vocab[i], i) != 0) {
            set_wasm_error("Failed to add token to vocabulary");
            hyperionDestroyTokenizer(g_wasm_context.tokenizer);
            hyperionConfigCleanup();
            hyperionMemoryCleanup();
            return -1;
        }
    }
    
    // Fill remaining vocabulary slots with generated tokens
    for (int i = vocab_count; i < vocab_size; i++) {
        char token[32];
        snprintf(token, sizeof(token), "tok_%d", i);
        hyperionAddTokenToVocabulary(g_wasm_context.tokenizer, token, i);
    }
    
    printf("Tokenizer created with %d tokens\n", vocab_size);
    
    // Create model
    g_wasm_context.model = hyperionCreateModel();
    if (!g_wasm_context.model) {
        set_wasm_error("Failed to create model");
        hyperionDestroyTokenizer(g_wasm_context.tokenizer);
        hyperionConfigCleanup();
        hyperionMemoryCleanup();
        return -1;
    }
    
    // Initialize model with ultra-lightweight configuration
    HyperionModelConfig model_config = {
        .vocabSize = vocab_size,
        .hiddenSize = hidden_size,
        .numLayers = num_layers,
        .maxSequenceLength = 256  // Conservative for browser memory
    };
    
    if (hyperionInitializeModel(g_wasm_context.model, &model_config) != 0) {
        set_wasm_error("Failed to initialize model");
        hyperionDestroyModel(g_wasm_context.model);
        hyperionDestroyTokenizer(g_wasm_context.tokenizer);
        hyperionConfigCleanup();
        hyperionMemoryCleanup();
        return -1;
    }
    
    printf("Model initialized (Hidden: %d, Layers: %d, Vocab: %d)\n", 
           hidden_size, num_layers, vocab_size);
    
    g_wasm_context.initialized = 1;
    
    size_t memory_usage = hyperionGetMemoryUsage();
    printf("Hyperion WASM initialized successfully! Memory usage: %.2f MB\n", 
           memory_usage / (1024.0 * 1024.0));
    
    return 0;
}

/**
 * Generate text using Hyperion in WebAssembly
 * 
 * @param prompt Input text prompt
 * @param max_tokens Maximum tokens to generate (0 = use default)
 * @param temperature Sampling temperature (0.0 = use default)
 * @return Generated text string (must be freed by caller with free())
 */
EMSCRIPTEN_KEEPALIVE
char* hyperion_wasm_generate_text(const char* prompt, int max_tokens, float temperature) {
    if (!g_wasm_context.initialized) {
        set_wasm_error("Hyperion not initialized. Call hyperion_wasm_init() first.");
        return NULL;
    }
    
    if (!prompt || strlen(prompt) == 0) {
        set_wasm_error("Empty prompt provided");
        return NULL;
    }
    
    // Use default parameters if not specified
    if (max_tokens <= 0) max_tokens = g_wasm_context.max_tokens;
    if (temperature <= 0.0f) temperature = g_wasm_context.temperature;
    
    // Limit parameters for browser environment
    if (max_tokens > 200) max_tokens = 200;
    if (temperature < 0.1f) temperature = 0.1f;
    if (temperature > 2.0f) temperature = 2.0f;
    
    printf("Generating text: prompt='%s', max_tokens=%d, temperature=%.2f\n", 
           prompt, max_tokens, temperature);
    
    // Tokenize input
    int prompt_tokens[256];
    int prompt_length = hyperionEncodeText(g_wasm_context.tokenizer, prompt, 
                                          prompt_tokens, 256);
    
    if (prompt_length <= 0) {
        set_wasm_error("Failed to tokenize input prompt");
        return NULL;
    }
    
    printf("Prompt tokenized: %d tokens\n", prompt_length);
    
    // Set up generation parameters
    HyperionGenerationParams params = {
        .maxTokens = max_tokens,
        .temperature = temperature,
        .samplingMethod = HYPERION_SAMPLING_TOP_P,
        .topP = 0.9f,
        .topK = 40,
        .promptTokens = prompt_tokens,
        .promptLength = prompt_length,
        .seed = (unsigned int)(emscripten_get_now() * 1000) // Use high-res timestamp
    };
    
    // Allocate output buffer
    int* output_tokens = (int*)malloc(max_tokens * sizeof(int));
    if (!output_tokens) {
        set_wasm_error("Failed to allocate memory for output tokens");
        return NULL;
    }
    
    // Generate text
    int generated_length = hyperionGenerateText(g_wasm_context.model, &params, 
                                               output_tokens, max_tokens);
    
    if (generated_length <= 0) {
        free(output_tokens);
        set_wasm_error("Text generation failed");
        return NULL;
    }
    
    printf("Generated %d tokens\n", generated_length);
    
    // Decode generated tokens
    char* output_text = (char*)malloc(generated_length * 20 + 1); // Conservative allocation
    if (!output_text) {
        free(output_tokens);
        set_wasm_error("Failed to allocate memory for output text");
        return NULL;
    }
    
    int decoded_length = hyperionDecodeTokens(g_wasm_context.tokenizer, 
                                             output_tokens, generated_length,
                                             output_text, generated_length * 20);
    
    free(output_tokens);
    
    if (decoded_length <= 0) {
        free(output_text);
        set_wasm_error("Failed to decode generated tokens");
        return NULL;
    }
    
    output_text[decoded_length] = '\0';
    printf("Generated text: '%s'\n", output_text);
    
    return output_text; // Caller must free this
}

/**
 * Get current memory usage statistics
 * 
 * @return JSON string with memory statistics
 */
EMSCRIPTEN_KEEPALIVE
char* hyperion_wasm_get_memory_stats(void) {
    if (!g_wasm_context.initialized) {
        return strdup("{\"error\":\"Not initialized\"}");
    }
    
    size_t current_usage = hyperionGetMemoryUsage();
    size_t allocation_count = hyperionGetAllocationCount();
    
    char* stats = (char*)malloc(512);
    if (!stats) return NULL;
    
    snprintf(stats, 512,
        "{"
        "\"current_usage_mb\":%.2f,"
        "\"memory_limit_mb\":%.2f,"
        "\"allocation_count\":%zu,"
        "\"initialized\":%s,"
        "\"max_tokens\":%d,"
        "\"temperature\":%.2f"
        "}",
        current_usage / (1024.0 * 1024.0),
        g_wasm_context.memory_limit / (1024.0 * 1024.0),
        allocation_count,
        g_wasm_context.initialized ? "true" : "false",
        g_wasm_context.max_tokens,
        g_wasm_context.temperature
    );
    
    return stats; // Caller must free this
}

/**
 * Update generation parameters
 */
EMSCRIPTEN_KEEPALIVE
void hyperion_wasm_set_params(int max_tokens, float temperature) {
    if (max_tokens > 0 && max_tokens <= 500) {
        g_wasm_context.max_tokens = max_tokens;
    }
    
    if (temperature > 0.0f && temperature <= 2.0f) {
        g_wasm_context.temperature = temperature;
    }
    
    printf("Updated parameters: max_tokens=%d, temperature=%.2f\n",
           g_wasm_context.max_tokens, g_wasm_context.temperature);
}

/**
 * Get last error message
 */
EMSCRIPTEN_KEEPALIVE
const char* hyperion_wasm_get_last_error(void) {
    return g_last_error;
}

/**
 * Clean up Hyperion WASM resources
 */
EMSCRIPTEN_KEEPALIVE
void hyperion_wasm_cleanup(void) {
    printf("Cleaning up Hyperion WASM resources...\n");
    
    if (g_wasm_context.model) {
        hyperionDestroyModel(g_wasm_context.model);
        g_wasm_context.model = NULL;
    }
    
    if (g_wasm_context.tokenizer) {
        hyperionDestroyTokenizer(g_wasm_context.tokenizer);
        g_wasm_context.tokenizer = NULL;
    }
    
    if (g_wasm_context.initialized) {
        hyperionConfigCleanup();
        hyperionMemoryCleanup();
        wasm_memory_cleanup();
    }
    
    memset(&g_wasm_context, 0, sizeof(g_wasm_context));
    memset(g_last_error, 0, sizeof(g_last_error));
    
    printf("Hyperion WASM cleanup completed\n");
}

/* Emscripten C++ bindings for easier JavaScript integration */
#ifdef __cplusplus
using namespace emscripten;

EMSCRIPTEN_BINDINGS(hyperion_wasm) {
    function("hyperion_wasm_init", &hyperion_wasm_init);
    function("hyperion_wasm_generate_text", &hyperion_wasm_generate_text, allow_raw_pointers());
    function("hyperion_wasm_get_memory_stats", &hyperion_wasm_get_memory_stats, allow_raw_pointers());
    function("hyperion_wasm_set_params", &hyperion_wasm_set_params);
    function("hyperion_wasm_get_last_error", &hyperion_wasm_get_last_error);
    function("hyperion_wasm_cleanup", &hyperion_wasm_cleanup);
}
#endif