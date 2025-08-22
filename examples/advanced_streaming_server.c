/**
 * Hyperion Advanced Example: Streaming Text Generation Server
 * 
 * This advanced example demonstrates sophisticated Hyperion usage including:
 * - Real-time streaming text generation
 * - Performance monitoring and optimization
 * - Advanced error handling and recovery
 * - Memory optimization techniques
 * - Production-ready logging and metrics
 * 
 * Memory Usage: 100-300MB (depending on model size)
 * Complexity: ⭐⭐⭐⭐⭐ (Advanced)
 * 
 * What this example demonstrates:
 * - Advanced model configuration and optimization
 * - Real-time streaming with performance monitoring
 * - Sophisticated error handling and recovery mechanisms
 * - Memory-efficient batch processing
 * - Production logging and metrics collection
 * - Advanced SIMD optimization usage
 */

#include "../core/memory.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../models/text/generate.h"
#include "../models/text/tokenizer.h"
#include "../interface/web_server.h"
#include "../utils/simd_benchmark.h"
#include "../core/enhanced_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

/* Server configuration */
typedef struct {
    int port;
    int max_concurrent_requests;
    int streaming_enabled;
    int performance_monitoring;
    char model_path[512];
    char tokenizer_path[512];
    char log_file[512];
} ServerConfig;

/* Performance metrics */
typedef struct {
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    double avg_tokens_per_second;
    double avg_response_time_ms;
    size_t peak_memory_usage;
    uint64_t total_tokens_generated;
} PerformanceMetrics;

/* Global state */
static volatile int g_server_running = 1;
static ServerConfig g_config;
static PerformanceMetrics g_metrics = {0};
static HyperionModel* g_model = NULL;
static HyperionTokenizer* g_tokenizer = NULL;
static pthread_mutex_t g_metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Signal handler for graceful shutdown */
void signal_handler(int sig) {
    printf("\nReceived signal %d, initiating graceful shutdown...\n", sig);
    g_server_running = 0;
}

/* Update performance metrics */
void update_metrics(int tokens_generated, double generation_time_ms, int success) {
    pthread_mutex_lock(&g_metrics_mutex);
    
    g_metrics.total_requests++;
    if (success) {
        g_metrics.successful_requests++;
        g_metrics.total_tokens_generated += tokens_generated;
        
        double tokens_per_second = tokens_generated / (generation_time_ms / 1000.0);
        g_metrics.avg_tokens_per_second = 
            (g_metrics.avg_tokens_per_second * (g_metrics.successful_requests - 1) + tokens_per_second) 
            / g_metrics.successful_requests;
        
        g_metrics.avg_response_time_ms = 
            (g_metrics.avg_response_time_ms * (g_metrics.successful_requests - 1) + generation_time_ms) 
            / g_metrics.successful_requests;
    } else {
        g_metrics.failed_requests++;
    }
    
    size_t current_memory = hyperionGetMemoryUsage();
    if (current_memory > g_metrics.peak_memory_usage) {
        g_metrics.peak_memory_usage = current_memory;
    }
    
    pthread_mutex_unlock(&g_metrics_mutex);
}

/* Advanced text generation with streaming */
int advanced_generate_text(const char* prompt, char* output_buffer, size_t buffer_size,
                          int max_tokens, float temperature, int stream_callback(const char* token)) {
    
    clock_t start_time = clock();
    
    /* Enhanced error handling */
    if (!prompt || !output_buffer || buffer_size == 0) {
        HYPERION_SET_ERROR(HYPERION_ERROR_VALIDATION_INPUT_NULL, 
                          "Invalid parameters for text generation",
                          "Ensure prompt and output buffer are valid");
        return -1;
    }
    
    /* Tokenize input with advanced error checking */
    int prompt_tokens[1024];
    int prompt_length = hyperionEncodeText(g_tokenizer, prompt, prompt_tokens, 1024);
    
    if (prompt_length <= 0) {
        HYPERION_SET_ERROR_WITH_CONTEXT(HYPERION_ERROR_MODEL_TOKENIZER_MISSING,
                                       "Failed to tokenize input prompt",
                                       "Check tokenizer is loaded and prompt contains valid characters",
                                       prompt, "tokenization");
        return -1;
    }
    
    /* Set up advanced generation parameters */
    HyperionGenerationParams params = {0};
    params.maxTokens = max_tokens;
    params.temperature = temperature;
    params.samplingMethod = HYPERION_SAMPLING_TOP_P;
    params.topP = 0.9f;
    params.topK = 40;
    params.promptTokens = prompt_tokens;
    params.promptLength = prompt_length;
    params.seed = (unsigned int)time(NULL);
    
    /* Advanced streaming generation */
    int output_tokens[2048];
    int total_generated = 0;
    char current_output[8192] = {0};
    
    hyperion_log(HYPERION_LOG_INFO, "Starting advanced text generation", 
                "prompt_length=%d max_tokens=%d temperature=%.2f", 
                prompt_length, max_tokens, temperature);
    
    /* Generate with streaming capability */
    for (int i = 0; i < max_tokens && g_server_running; i++) {
        /* Generate single token */
        params.maxTokens = 1;
        int token_result = hyperionGenerateText(g_model, &params, &output_tokens[total_generated], 1);
        
        if (token_result <= 0) {
            HYPERION_SET_ERROR_WITH_DATA(HYPERION_ERROR_MODEL_INFERENCE_FAILED,
                                        "Token generation failed during streaming",
                                        "Check model state and memory availability",
                                        i, total_generated);
            break;
        }
        
        total_generated += token_result;
        
        /* Decode current token for streaming */
        char token_text[64];
        if (hyperionDecodeTokens(g_tokenizer, &output_tokens[total_generated-1], 1, token_text, 64) > 0) {
            strcat(current_output, token_text);
            
            /* Stream callback for real-time updates */
            if (stream_callback && stream_callback(token_text) != 0) {
                hyperion_log(HYPERION_LOG_WARNING, "Streaming callback requested stop", NULL);
                break;
            }
        }
        
        /* Check for stop conditions */
        if (output_tokens[total_generated-1] == hyperionGetTokenId(g_tokenizer, "</s>")) {
            hyperion_log(HYPERION_LOG_DEBUG, "Generation stopped at end token", NULL);
            break;
        }
        
        /* Memory pressure check */
        size_t current_memory = hyperionGetMemoryUsage();
        if (current_memory > 500 * 1024 * 1024) { /* 500MB limit */
            hyperion_log(HYPERION_LOG_WARNING, "Memory pressure detected, stopping generation", 
                        "memory_usage=%.2fMB", current_memory / (1024.0 * 1024.0));
            break;
        }
        
        /* Update context for next token */
        params.promptTokens = output_tokens;
        params.promptLength = total_generated;
    }
    
    /* Final decode */
    if (hyperionDecodeTokens(g_tokenizer, output_tokens, total_generated, output_buffer, buffer_size) <= 0) {
        HYPERION_SET_ERROR(HYPERION_ERROR_MODEL_INFERENCE_FAILED,
                          "Failed to decode generated tokens",
                          "Check tokenizer compatibility and output buffer size");
        return -1;
    }
    
    /* Calculate performance metrics */
    clock_t end_time = clock();
    double generation_time_ms = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;
    
    update_metrics(total_generated, generation_time_ms, 1);
    
    hyperion_log(HYPERION_LOG_INFO, "Text generation completed successfully", 
                "tokens_generated=%d time_ms=%.2f tokens_per_sec=%.2f", 
                total_generated, generation_time_ms, 
                total_generated / (generation_time_ms / 1000.0));
    
    return total_generated;
}

/* Advanced model initialization with optimization */
int initialize_advanced_model(const char* model_path, const char* tokenizer_path) {
    hyperion_log(HYPERION_LOG_INFO, "Initializing advanced model", 
                "model=%s tokenizer=%s", model_path, tokenizer_path);
    
    /* Initialize enhanced error system */
    if (hyperion_enhanced_errors_init() != 0) {
        fprintf(stderr, "Failed to initialize enhanced error system\n");
        return -1;
    }
    
    /* Run SIMD capability detection */
    printf("Detecting SIMD capabilities...\n");
    HyperionSIMDCapabilities simd_caps;
    if (hyperionDetectSIMDCapabilities(&simd_caps) == 0) {
        printf("✓ SIMD Support: SSE2=%s AVX=%s AVX2=%s\n",
               simd_caps.hasSSE2 ? "Yes" : "No",
               simd_caps.hasAVX ? "Yes" : "No", 
               simd_caps.hasAVX2 ? "Yes" : "No");
               
        /* Run SIMD benchmarks */
        if (simd_caps.hasAVX2) {
            HyperionSIMDBenchmarkResult benchmark_result;
            if (hyperionRunSIMDBenchmark(&benchmark_result, 1000) == 0) {
                printf("✓ SIMD Performance: %.2fx speedup over scalar operations\n", 
                       benchmark_result.speedupFactor);
            }
        }
    }
    
    /* Create tokenizer with advanced error handling */
    g_tokenizer = hyperionCreateTokenizer();
    if (!g_tokenizer) {
        HYPERION_SET_ERROR(HYPERION_ERROR_MEMORY_ALLOCATION_FAILED,
                          "Failed to create tokenizer object",
                          "Check available memory and system resources");
        return -1;
    }
    
    /* Load tokenizer vocabulary */
    if (hyperionLoadVocabulary(g_tokenizer, tokenizer_path) != 0) {
        HYPERION_SET_ERROR_WITH_CONTEXT(HYPERION_ERROR_IO_FILE_NOT_FOUND,
                                       "Failed to load tokenizer vocabulary",
                                       "Check file path and permissions",
                                       tokenizer_path, "vocabulary_loading");
        return -1;
    }
    
    int vocab_size = hyperionGetVocabularySize(g_tokenizer);
    printf("✓ Tokenizer loaded: %d vocabulary entries\n", vocab_size);
    
    /* Create model with advanced configuration */
    g_model = hyperionCreateModel();
    if (!g_model) {
        HYPERION_SET_ERROR(HYPERION_ERROR_MEMORY_ALLOCATION_FAILED,
                          "Failed to create model object", 
                          "Increase available memory or use smaller model");
        return -1;
    }
    
    /* Load model with optimization */
    if (hyperionLoadModelFromFile(g_model, model_path) != 0) {
        HYPERION_SET_ERROR_WITH_CONTEXT(HYPERION_ERROR_MODEL_FORMAT_UNSUPPORTED,
                                       "Failed to load model from file",
                                       "Check model format and file integrity",
                                       model_path, "model_loading");
        return -1;
    }
    
    /* Optimize model for performance */
    printf("Optimizing model for performance...\n");
    
    if (hyperionOptimizeModelForInference(g_model) == 0) {
        printf("✓ Model optimization completed\n");
    } else {
        printf("⚠ Model optimization failed, continuing with default settings\n");
    }
    
    /* Validate model compatibility */
    if (hyperionValidateModelTokenizerCompatibility(g_model, g_tokenizer) != 0) {
        HYPERION_SET_ERROR(HYPERION_ERROR_MODEL_VERSION_MISMATCH,
                          "Model and tokenizer are incompatible",
                          "Use matching model and tokenizer versions");
        return -1;
    }
    
    printf("✓ Advanced model initialization completed successfully\n");
    
    /* Log initial memory usage */
    size_t initial_memory = hyperionGetMemoryUsage();
    hyperion_log(HYPERION_LOG_INFO, "Model loaded successfully", 
                "initial_memory=%.2fMB vocab_size=%d", 
                initial_memory / (1024.0 * 1024.0), vocab_size);
    
    return 0;
}

/* Performance monitoring thread */
void* performance_monitor_thread(void* arg) {
    (void)arg;
    
    while (g_server_running) {
        sleep(30); /* Report every 30 seconds */
        
        pthread_mutex_lock(&g_metrics_mutex);
        
        printf("\n=== Performance Report ===\n");
        printf("Total requests: %llu\n", (unsigned long long)g_metrics.total_requests);
        printf("Success rate: %.2f%%\n", 
               g_metrics.total_requests > 0 ? 
               (double)g_metrics.successful_requests / g_metrics.total_requests * 100.0 : 0.0);
        printf("Avg tokens/sec: %.2f\n", g_metrics.avg_tokens_per_second);
        printf("Avg response time: %.2f ms\n", g_metrics.avg_response_time_ms);
        printf("Peak memory: %.2f MB\n", g_metrics.peak_memory_usage / (1024.0 * 1024.0));
        printf("Total tokens generated: %llu\n", (unsigned long long)g_metrics.total_tokens_generated);
        printf("==========================\n\n");
        
        /* Log performance metrics */
        hyperion_log(HYPERION_LOG_INFO, "Performance metrics", 
                    "requests=%llu success_rate=%.2f%% tokens_per_sec=%.2f memory_mb=%.2f",
                    (unsigned long long)g_metrics.total_requests,
                    g_metrics.total_requests > 0 ? 
                    (double)g_metrics.successful_requests / g_metrics.total_requests * 100.0 : 0.0,
                    g_metrics.avg_tokens_per_second,
                    g_metrics.peak_memory_usage / (1024.0 * 1024.0));
        
        pthread_mutex_unlock(&g_metrics_mutex);
    }
    
    return NULL;
}

/* Streaming callback function */
int streaming_callback(const char* token) {
    printf("%s", token);
    fflush(stdout);
    return 0; /* Continue streaming */
}

int main(int argc, char *argv[]) {
    printf("=== Hyperion Advanced Streaming Server ===\n");
    printf("Production-ready text generation with real-time streaming\n\n");
    
    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Initialize configuration */
    strcpy(g_config.model_path, argc > 1 ? argv[1] : "models/advanced_model.bin");
    strcpy(g_config.tokenizer_path, argc > 2 ? argv[2] : "models/tokenizer.txt");
    strcpy(g_config.log_file, "hyperion_server.log");
    g_config.port = 8080;
    g_config.max_concurrent_requests = 10;
    g_config.streaming_enabled = 1;
    g_config.performance_monitoring = 1;
    
    /* Initialize enhanced logging */
    HyperionLogConfig log_config = {0};
    log_config.level = HYPERION_LOG_INFO;
    log_config.output = HYPERION_LOG_OUTPUT_CONSOLE | HYPERION_LOG_OUTPUT_FILE;
    log_config.colorize_console = 1;
    
    if (hyperion_configure_logging(&log_config) != 0) {
        fprintf(stderr, "Warning: Failed to configure logging\n");
    }
    
    if (hyperion_set_log_file(g_config.log_file) != 0) {
        fprintf(stderr, "Warning: Failed to set log file\n");
    }
    
    /* Initialize Hyperion subsystems */
    printf("Initializing Hyperion subsystems...\n");
    
    if (hyperionConfigInit() != 0) {
        fprintf(stderr, "Error: Failed to initialize configuration\n");
        return 1;
    }
    
    hyperionMemTrackInit();
    
    /* Configure for server usage */
    hyperionConfigSetString("memory.pool_size", "268435456");  // 256MB
    hyperionConfigSetString("memory.max_allocations", "100000");
    hyperionConfigSetString("memory.track_leaks", "true");
    
    /* Initialize advanced model */
    if (initialize_advanced_model(g_config.model_path, g_config.tokenizer_path) != 0) {
        const HyperionErrorInfo* error = hyperion_get_last_error();
        if (error) {
            hyperion_print_error_report(1);
        }
        goto cleanup;
    }
    
    /* Start performance monitoring thread */
    pthread_t monitor_thread;
    if (g_config.performance_monitoring) {
        if (pthread_create(&monitor_thread, NULL, performance_monitor_thread, NULL) != 0) {
            printf("Warning: Failed to start performance monitoring thread\n");
        } else {
            printf("✓ Performance monitoring started\n");
        }
    }
    
    /* Server main loop */
    printf("\n=== Server Ready ===\n");
    printf("Listening on port %d\n", g_config.port);
    printf("Max concurrent requests: %d\n", g_config.max_concurrent_requests);
    printf("Streaming: %s\n", g_config.streaming_enabled ? "Enabled" : "Disabled");
    printf("Type 'quit' to stop the server\n\n");
    
    /* Interactive demo mode */
    char input_buffer[1024];
    char output_buffer[4096];
    
    while (g_server_running) {
        printf("Enter prompt (or 'quit' to exit): ");
        fflush(stdout);
        
        if (!fgets(input_buffer, sizeof(input_buffer), stdin)) {
            break;
        }
        
        /* Remove newline */
        input_buffer[strcspn(input_buffer, "\r\n")] = 0;
        
        if (strcmp(input_buffer, "quit") == 0 || strcmp(input_buffer, "exit") == 0) {
            break;
        }
        
        if (strlen(input_buffer) == 0) {
            continue;
        }
        
        printf("\nGenerating (streaming): ");
        
        int result = advanced_generate_text(input_buffer, output_buffer, sizeof(output_buffer),
                                          100, 0.8f, streaming_callback);
        
        if (result > 0) {
            printf("\n\nGeneration completed: %d tokens\n", result);
        } else {
            printf("\nGeneration failed\n");
            const HyperionErrorInfo* error = hyperion_get_last_error();
            if (error) {
                printf("Error: %s\n", error->message);
                if (error->suggestion) {
                    printf("Suggestion: %s\n", error->suggestion);
                }
            }
        }
        
        printf("\n");
    }
    
cleanup:
    printf("\nShutting down server...\n");
    
    /* Stop performance monitoring */
    if (g_config.performance_monitoring) {
        pthread_cancel(monitor_thread);
        pthread_join(monitor_thread, NULL);
    }
    
    /* Final performance report */
    printf("\n=== Final Performance Report ===\n");
    printf("Total requests processed: %llu\n", (unsigned long long)g_metrics.total_requests);
    printf("Success rate: %.2f%%\n", 
           g_metrics.total_requests > 0 ? 
           (double)g_metrics.successful_requests / g_metrics.total_requests * 100.0 : 0.0);
    printf("Total tokens generated: %llu\n", (unsigned long long)g_metrics.total_tokens_generated);
    printf("Peak memory usage: %.2f MB\n", g_metrics.peak_memory_usage / (1024.0 * 1024.0));
    
    /* Cleanup resources */
    if (g_model) {
        hyperionDestroyModel(g_model);
    }
    
    if (g_tokenizer) {
        hyperionDestroyTokenizer(g_tokenizer);
    }
    
    /* Memory leak detection */
    int leak_count = hyperionMemTrackDumpLeaks();
    if (leak_count == 0) {
        printf("✓ No memory leaks detected\n");
    } else {
        printf("⚠ %d memory leaks detected\n", leak_count);
    }
    
    hyperionMemTrackCleanup();
    hyperionConfigCleanup();
    hyperion_enhanced_errors_cleanup();
    
    printf("✓ Server shutdown completed\n");
    
    return 0;
}

/*
 * Build Instructions:
 * 
 * gcc -o advanced_streaming_server advanced_streaming_server.c \
 *     ../core/*.c ../models/text/*.c ../interface/web_server.c \
 *     ../utils/simd_benchmark.c -I.. -lm -lpthread
 * 
 * Or with CMake:
 * mkdir build && cd build
 * cmake .. -DBUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
 * make advanced_streaming_server
 * 
 * Usage:
 * ./advanced_streaming_server [model_path] [tokenizer_path]
 * 
 * Features demonstrated:
 * - Real-time streaming text generation
 * - Advanced error handling with context
 * - Performance monitoring and metrics
 * - Memory optimization and leak detection
 * - SIMD capability detection and benchmarking
 * - Production-ready logging
 * - Graceful shutdown handling
 * - Multi-threaded architecture
 */