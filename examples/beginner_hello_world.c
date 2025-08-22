/**
 * Hyperion Beginner Example: Hello World Text Generation
 * 
 * This is the simplest possible example of using Hyperion for AI text generation.
 * Perfect for first-time users to understand basic concepts.
 * 
 * Memory Usage: ~16MB
 * Complexity: ⭐☆☆☆☆ (Beginner)
 * 
 * What this example demonstrates:
 * - Basic Hyperion initialization
 * - Loading a simple model
 * - Basic text generation
 * - Proper cleanup
 */

#include "../core/memory.h"
#include "../core/config.h"
#include "../models/text/generate.h"
#include "../models/text/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf("=== Hyperion Hello World Example ===\n");
    printf("This example demonstrates basic text generation.\n\n");
    
    // Step 1: Initialize Hyperion subsystems
    printf("Step 1: Initializing Hyperion...\n");
    
    if (hyperionConfigInit() != 0) {
        fprintf(stderr, "Error: Failed to initialize configuration system\n");
        return 1;
    }
    
    hyperionMemTrackInit();
    
    printf("✓ Hyperion initialized successfully\n\n");
    
    // Step 2: Set up basic configuration
    printf("Step 2: Setting up configuration...\n");
    
    // Use minimal memory settings for this example
    hyperionConfigSetString("memory.pool_size", "16777216");  // 16MB
    hyperionConfigSetString("model.context_size", "128");     // Small context
    hyperionConfigSetString("model.temperature", "0.7");      // Moderate creativity
    
    printf("✓ Configuration set for minimal memory usage\n\n");
    
    // Step 3: Create a simple tokenizer
    printf("Step 3: Creating tokenizer...\n");
    
    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
    if (!tokenizer) {
        fprintf(stderr, "Error: Failed to create tokenizer\n");
        goto cleanup;
    }
    
    // For this example, we'll use a very simple vocabulary
    // In a real application, you'd load this from a file
    const char *simple_vocab[] = {
        "<pad>", "<unk>", "<s>", "</s>",
        "hello", "world", "the", "is", "a", "an", "and", "or", "but",
        "I", "you", "he", "she", "it", "we", "they",
        "good", "bad", "nice", "great", "awesome", "wonderful",
        ".", "!", "?", ",", " "
    };
    
    int vocab_size = sizeof(simple_vocab) / sizeof(simple_vocab[0]);
    for (int i = 0; i < vocab_size; i++) {
        hyperionAddTokenToVocabulary(tokenizer, simple_vocab[i], i);
    }
    
    printf("✓ Simple tokenizer created with %d tokens\n\n", vocab_size);
    
    // Step 4: Create a minimal model
    printf("Step 4: Creating minimal text generation model...\n");
    
    HyperionModel *model = hyperionCreateModel();
    if (!model) {
        fprintf(stderr, "Error: Failed to create model\n");
        goto cleanup;
    }
    
    // Initialize with minimal parameters for this example
    HyperionModelConfig config = {0};
    config.vocabSize = vocab_size;
    config.hiddenSize = 64;      // Very small for minimal memory usage
    config.numLayers = 2;        // Just 2 layers
    config.contextSize = 128;    // Small context window
    config.quantizationBits = 4; // Use 4-bit quantization
    
    if (hyperionInitializeModel(model, &config) != 0) {
        fprintf(stderr, "Error: Failed to initialize model\n");
        goto cleanup;
    }
    
    printf("✓ Minimal model created (Hidden: %d, Layers: %d, Vocab: %d)\n\n", 
           config.hiddenSize, config.numLayers, config.vocabSize);
    
    // Step 5: Generate some text
    printf("Step 5: Generating text...\n");
    printf("Prompt: \"Hello\"\n");
    printf("Generated text: ");
    
    // Set up generation parameters
    HyperionGenerationParams params = {0};
    params.maxTokens = 10;           // Generate just 10 tokens
    params.temperature = 0.7f;       // Moderate randomness
    params.samplingMethod = HYPERION_SAMPLING_TEMPERATURE;
    params.topK = 5;                 // Consider top 5 tokens
    params.seed = 42;                // Fixed seed for reproducible results
    
    // Tokenize the prompt "hello"
    int promptTokens[16];
    int promptLength = hyperionEncodeText(tokenizer, "hello", promptTokens, 16);
    
    if (promptLength <= 0) {
        fprintf(stderr, "Error: Failed to tokenize prompt\n");
        goto cleanup;
    }
    
    params.promptTokens = promptTokens;
    params.promptLength = promptLength;
    
    // Generate text
    int outputTokens[32];
    int outputLength = hyperionGenerateText(model, &params, outputTokens, 32);
    
    if (outputLength > 0) {
        // Decode and print the generated text
        char outputText[512];
        if (hyperionDecodeTokens(tokenizer, outputTokens, outputLength, outputText, 512) > 0) {
            printf("%s\n", outputText);
        } else {
            printf("(decoding failed)\n");
        }
    } else {
        printf("(generation failed)\n");
    }
    
    printf("\n");
    
    // Step 6: Show memory usage
    printf("Step 6: Memory usage summary...\n");
    
    size_t currentMemory = hyperionGetMemoryUsage();
    printf("✓ Current memory usage: %.2f MB\n", currentMemory / (1024.0 * 1024.0));
    printf("✓ Memory tracking: %d allocations\n", hyperionMemTrackGetAllocCount());
    
    printf("\n=== Example Completed Successfully! ===\n");
    printf("Next steps:\n");
    printf("1. Try modifying the prompt in the code\n");
    printf("2. Experiment with different generation parameters\n");
    printf("3. Look at intermediate examples for more advanced features\n");
    printf("4. Check examples/chatbot/ for a complete interactive application\n\n");
    
cleanup:
    // Step 7: Clean up resources
    printf("Cleaning up resources...\n");
    
    if (model) {
        hyperionDestroyModel(model);
    }
    
    if (tokenizer) {
        hyperionDestroyTokenizer(tokenizer);
    }
    
    // Check for memory leaks
    int leakCount = hyperionMemTrackDumpLeaks();
    if (leakCount == 0) {
        printf("✓ No memory leaks detected\n");
    } else {
        printf("⚠ %d memory leaks detected (see above)\n", leakCount);
    }
    
    hyperionMemTrackCleanup();
    hyperionConfigCleanup();
    
    printf("✓ Cleanup completed\n");
    
    return 0;
}

/* 
 * Build Instructions:
 * 
 * gcc -o beginner_hello_world beginner_hello_world.c \
 *     ../core/memory.c ../core/config.c \
 *     ../models/text/generate.c ../models/text/tokenizer.c \
 *     -I.. -lm
 * 
 * Or use CMake:
 * mkdir build && cd build
 * cmake .. -DBUILD_EXAMPLES=ON
 * make beginner_hello_world
 * 
 * Run:
 * ./beginner_hello_world
 */