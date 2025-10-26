/**
 * Hyperion Performance Benchmarking Suite
 * 
 * Standardized performance measurement and validation for the Hyperion AI framework.
 * Measures inference speed, memory efficiency, and optimization effectiveness.
 */

#include "../core/memory.h"
#include "../core/config.h"
#include "../models/text/generate.h"
#include "../models/text/tokenizer.h"
#include "../core/enhanced_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

/* Benchmark configuration */
#define BENCHMARK_ITERATIONS 100
#define WARMUP_ITERATIONS 10
#define MIN_TOKENS_FOR_BENCHMARK 10
#define MAX_TOKENS_FOR_BENCHMARK 1000

/* Performance metrics */
typedef struct {
    double tokens_per_second;
    double memory_efficiency_mb_per_token;
    double initialization_time_ms;
    double generation_time_ms;
    double cleanup_time_ms;
    size_t peak_memory_usage;
    size_t average_memory_usage;
    int successful_generations;
    int failed_generations;
} PerformanceMetrics;

/* Benchmark configuration variants */
typedef struct {
    const char* name;
    int vocab_size;
    int hidden_size;
    int num_layers;
    int max_sequence_length;
    int max_tokens;
    float temperature;
} BenchmarkConfig;

/* Standard benchmark configurations */
static const BenchmarkConfig BENCHMARK_CONFIGS[] = {
    {
        "Ultra-Light (Embedded)",
        500, 32, 2, 128, 50, 0.7f
    },
    {
        "Light (Mobile)",
        1000, 64, 4, 256, 100, 0.7f
    },
    {
        "Standard (Desktop)",
        5000, 128, 6, 512, 200, 0.7f
    },
    {
        "Large (Server)",
        10000, 256, 8, 1024, 500, 0.7f
    }
};

#define NUM_BENCHMARK_CONFIGS (sizeof(BENCHMARK_CONFIGS) / sizeof(BENCHMARK_CONFIGS[0]))

/* Color output */
#ifdef _WIN32
#define COLOR_GREEN ""
#define COLOR_RED ""
#define COLOR_YELLOW ""
#define COLOR_BLUE ""
#define COLOR_CYAN ""
#define COLOR_RESET ""
#else
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_CYAN "\033[36m"
#define COLOR_RESET "\033[0m"
#endif

/* Utility functions */
static double get_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

static size_t get_process_memory_usage(void) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024;
    }
    return 0;
#endif
}

static void print_header(const char* title) {
    printf("\n%s=== %s ===%s\n", COLOR_BLUE, title, COLOR_RESET);
}

static void print_config_info(const BenchmarkConfig* config) {
    printf("%sConfiguration: %s%s\n", COLOR_CYAN, config->name, COLOR_RESET);
    printf("  Vocabulary: %d tokens\n", config->vocab_size);
    printf("  Hidden size: %d\n", config->hidden_size);
    printf("  Layers: %d\n", config->num_layers);
    printf("  Max sequence: %d\n", config->max_sequence_length);
    printf("  Max tokens: %d\n", config->max_tokens);
    printf("  Temperature: %.1f\n", config->temperature);
}

static void print_metrics(const PerformanceMetrics* metrics) {
    printf("\n%sPerformance Results:%s\n", COLOR_GREEN, COLOR_RESET);
    printf("  🚀 Tokens/second: %.2f\n", metrics->tokens_per_second);
    printf("  💾 Memory efficiency: %.3f MB/token\n", metrics->memory_efficiency_mb_per_token);
    printf("  ⏱️  Initialization: %.2f ms\n", metrics->initialization_time_ms);
    printf("  🎯 Generation: %.2f ms\n", metrics->generation_time_ms);
    printf("  🧹 Cleanup: %.2f ms\n", metrics->cleanup_time_ms);
    printf("  📊 Peak memory: %.2f MB\n", metrics->peak_memory_usage / (1024.0 * 1024.0));
    printf("  📈 Average memory: %.2f MB\n", metrics->average_memory_usage / (1024.0 * 1024.0));
    printf("  ✅ Success rate: %.1f%% (%d/%d)\n", 
           100.0 * metrics->successful_generations / (metrics->successful_generations + metrics->failed_generations),
           metrics->successful_generations, 
           metrics->successful_generations + metrics->failed_generations);
}

/* Create and initialize model for benchmarking */
static int setup_benchmark_model(const BenchmarkConfig* config, 
                                HyperionModel** model, 
                                HyperionTokenizer** tokenizer,
                                PerformanceMetrics* metrics) {
    
    double start_time = get_time_ms();
    
    /* Create tokenizer */
    *tokenizer = hyperionCreateTokenizer();
    if (!*tokenizer) {
        return -1;
    }
    
    /* Add vocabulary */
    for (int i = 0; i < config->vocab_size; i++) {
        char token[32];
        snprintf(token, sizeof(token), "token_%d", i);
        if (hyperionAddTokenToVocabulary(*tokenizer, token, i) != 0) {
            hyperionDestroyTokenizer(*tokenizer);
            return -1;
        }
    }
    
    /* Create model */
    *model = hyperionCreateModel();
    if (!*model) {
        hyperionDestroyTokenizer(*tokenizer);
        return -1;
    }
    
    /* Initialize model */
    HyperionModelConfig model_config = {
        .vocabSize = config->vocab_size,
        .hiddenSize = config->hidden_size,
        .numLayers = config->num_layers,
        .maxSequenceLength = config->max_sequence_length
    };
    
    if (hyperionInitializeModel(*model, &model_config) != 0) {
        hyperionDestroyModel(*model);
        hyperionDestroyTokenizer(*tokenizer);
        return -1;
    }
    
    double end_time = get_time_ms();
    metrics->initialization_time_ms = end_time - start_time;
    
    return 0;
}

/* Run single benchmark iteration */
static int run_benchmark_iteration(HyperionModel* model, 
                                  HyperionTokenizer* tokenizer,
                                  const BenchmarkConfig* config,
                                  int* tokens_generated,
                                  double* generation_time) {
    
    /* Create prompt */
    int prompt_tokens[] = {1, 2, 3, 4, 5}; /* Simple test prompt */
    int prompt_length = 5;
    
    /* Prepare generation parameters */
    HyperionGenerationParams params = {
        .maxTokens = config->max_tokens,
        .temperature = config->temperature,
        .samplingMethod = HYPERION_SAMPLING_TOP_P,
        .topP = 0.9f,
        .topK = 40,
        .promptTokens = prompt_tokens,
        .promptLength = prompt_length,
        .seed = (unsigned int)time(NULL) + rand()
    };
    
    /* Allocate output buffer */
    int* output_tokens = malloc(config->max_tokens * sizeof(int));
    if (!output_tokens) {
        return -1;
    }
    
    /* Run generation */
    double start_time = get_time_ms();
    
    int generated = hyperionGenerateText(model, &params, output_tokens, config->max_tokens);
    
    double end_time = get_time_ms();
    
    free(output_tokens);
    
    if (generated <= 0) {
        return -1;
    }
    
    *tokens_generated = generated;
    *generation_time = end_time - start_time;
    
    return 0;
}

/* Run complete benchmark for a configuration */
static int run_configuration_benchmark(const BenchmarkConfig* config, PerformanceMetrics* metrics) {
    print_header("Running Benchmark");
    print_config_info(config);
    
    /* Initialize metrics */
    memset(metrics, 0, sizeof(PerformanceMetrics));
    
    /* Setup model and tokenizer */
    HyperionModel* model = NULL;
    HyperionTokenizer* tokenizer = NULL;
    
    if (setup_benchmark_model(config, &model, &tokenizer, metrics) != 0) {
        printf("%sERROR:%s Failed to setup benchmark model\n", COLOR_RED, COLOR_RESET);
        return -1;
    }
    
    printf("✅ Model and tokenizer initialized (%.2f ms)\n", metrics->initialization_time_ms);
    
    /* Warmup runs */
    printf("🔥 Running warmup iterations...\n");
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        int tokens_generated;
        double generation_time;
        
        if (run_benchmark_iteration(model, tokenizer, config, &tokens_generated, &generation_time) == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    printf(" warmup complete\n");
    
    /* Benchmark runs */
    printf("⚡ Running benchmark iterations...\n");
    
    size_t memory_sum = 0;
    int memory_measurements = 0;
    double total_generation_time = 0;
    int total_tokens_generated = 0;
    
    double benchmark_start = get_time_ms();
    
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        int tokens_generated;
        double generation_time;
        
        size_t memory_before = get_process_memory_usage();
        
        if (run_benchmark_iteration(model, tokenizer, config, &tokens_generated, &generation_time) == 0) {
            metrics->successful_generations++;
            total_tokens_generated += tokens_generated;
            total_generation_time += generation_time;
            
            /* Memory tracking */
            size_t memory_after = get_process_memory_usage();
            if (memory_after > metrics->peak_memory_usage) {
                metrics->peak_memory_usage = memory_after;
            }
            
            memory_sum += memory_after;
            memory_measurements++;
            
            if (i % 10 == 0) {
                printf(".");
                fflush(stdout);
            }
        } else {
            metrics->failed_generations++;
        }
    }
    
    double benchmark_end = get_time_ms();
    printf(" benchmark complete\n");
    
    /* Calculate metrics */
    if (metrics->successful_generations > 0) {
        metrics->generation_time_ms = total_generation_time / metrics->successful_generations;
        metrics->tokens_per_second = total_tokens_generated * 1000.0 / total_generation_time;
        
        if (memory_measurements > 0) {
            metrics->average_memory_usage = memory_sum / memory_measurements;
            metrics->memory_efficiency_mb_per_token = 
                (metrics->average_memory_usage / (1024.0 * 1024.0)) / 
                (total_tokens_generated / (double)metrics->successful_generations);
        }
    }
    
    /* Cleanup timing */
    double cleanup_start = get_time_ms();
    hyperionDestroyModel(model);
    hyperionDestroyTokenizer(tokenizer);
    double cleanup_end = get_time_ms();
    
    metrics->cleanup_time_ms = cleanup_end - cleanup_start;
    
    return 0;
}

/* Performance classification */
static const char* classify_performance(double tokens_per_second) {
    if (tokens_per_second >= 1000) return "Excellent";
    if (tokens_per_second >= 500) return "Very Good";
    if (tokens_per_second >= 100) return "Good";
    if (tokens_per_second >= 50) return "Fair";
    return "Needs Improvement";
}

static const char* classify_memory_efficiency(double mb_per_token) {
    if (mb_per_token <= 0.001) return "Excellent";
    if (mb_per_token <= 0.005) return "Very Good";
    if (mb_per_token <= 0.01) return "Good";
    if (mb_per_token <= 0.05) return "Fair";
    return "Needs Improvement";
}

/* Main benchmark runner */
int main(int argc, char* argv[]) {
    printf("%s=== Hyperion Performance Benchmarking Suite ===%s\n", COLOR_BLUE, COLOR_RESET);
    printf("Standardized performance measurement and validation\n");
    printf("Framework: Hyperion ultra-lightweight AI with 4-bit quantization\n\n");
    
    /* Initialize random seed */
    srand((unsigned int)time(NULL));
    
    /* Initialize Hyperion */
    if (hyperionMemoryInit() != 0) {
        printf("%sERROR:%s Failed to initialize Hyperion memory system\n", COLOR_RED, COLOR_RESET);
        return 1;
    }
    
    if (hyperionConfigInit() != 0) {
        printf("%sERROR:%s Failed to initialize Hyperion configuration\n", COLOR_RED, COLOR_RESET);
        return 1;
    }
    
    hyperion_enhanced_errors_init();
    
    /* Run benchmarks for each configuration */
    PerformanceMetrics results[NUM_BENCHMARK_CONFIGS];
    int successful_benchmarks = 0;
    
    for (int i = 0; i < NUM_BENCHMARK_CONFIGS; i++) {
        printf("\n%s[%d/%d] Benchmarking: %s%s\n", 
               COLOR_YELLOW, i + 1, NUM_BENCHMARK_CONFIGS, 
               BENCHMARK_CONFIGS[i].name, COLOR_RESET);
        
        if (run_configuration_benchmark(&BENCHMARK_CONFIGS[i], &results[i]) == 0) {
            print_metrics(&results[i]);
            
            /* Performance assessment */
            const char* speed_rating = classify_performance(results[i].tokens_per_second);
            const char* memory_rating = classify_memory_efficiency(results[i].memory_efficiency_mb_per_token);
            
            printf("  📊 Speed rating: %s%s%s\n", COLOR_CYAN, speed_rating, COLOR_RESET);
            printf("  📊 Memory rating: %s%s%s\n", COLOR_CYAN, memory_rating, COLOR_RESET);
            
            successful_benchmarks++;
        } else {
            printf("%s❌ Benchmark failed for %s%s\n", COLOR_RED, BENCHMARK_CONFIGS[i].name, COLOR_RESET);
        }
    }
    
    /* Summary report */
    print_header("Benchmark Summary");
    
    if (successful_benchmarks > 0) {
        printf("✅ Successfully benchmarked %d/%d configurations\n\n", 
               successful_benchmarks, NUM_BENCHMARK_CONFIGS);
        
        /* Find best performing configuration */
        int best_speed_idx = 0;
        int best_memory_idx = 0;
        
        for (int i = 1; i < successful_benchmarks; i++) {
            if (results[i].tokens_per_second > results[best_speed_idx].tokens_per_second) {
                best_speed_idx = i;
            }
            if (results[i].memory_efficiency_mb_per_token < results[best_memory_idx].memory_efficiency_mb_per_token) {
                best_memory_idx = i;
            }
        }
        
        printf("%s🏆 Best Speed: %s%s (%.2f tokens/sec)\n", 
               COLOR_GREEN, BENCHMARK_CONFIGS[best_speed_idx].name, COLOR_RESET,
               results[best_speed_idx].tokens_per_second);
        
        printf("%s🏆 Best Memory Efficiency: %s%s (%.3f MB/token)\n", 
               COLOR_GREEN, BENCHMARK_CONFIGS[best_memory_idx].name, COLOR_RESET,
               results[best_memory_idx].memory_efficiency_mb_per_token);
        
        /* Overall assessment */
        double avg_speed = 0;
        double avg_memory_eff = 0;
        
        for (int i = 0; i < successful_benchmarks; i++) {
            avg_speed += results[i].tokens_per_second;
            avg_memory_eff += results[i].memory_efficiency_mb_per_token;
        }
        
        avg_speed /= successful_benchmarks;
        avg_memory_eff /= successful_benchmarks;
        
        printf("\n%s📊 Overall Performance:%s\n", COLOR_BLUE, COLOR_RESET);
        printf("  Average speed: %.2f tokens/sec (%s)\n", 
               avg_speed, classify_performance(avg_speed));
        printf("  Average memory efficiency: %.3f MB/token (%s)\n", 
               avg_memory_eff, classify_memory_efficiency(avg_memory_eff));
        
        if (avg_speed > 100 && avg_memory_eff < 0.01) {
            printf("\n%s🎉 EXCELLENT: Hyperion performance meets ultra-lightweight criteria!%s\n", 
                   COLOR_GREEN, COLOR_RESET);
        } else if (avg_speed > 50 && avg_memory_eff < 0.05) {
            printf("\n%s✅ GOOD: Hyperion performance is acceptable for lightweight AI%s\n", 
                   COLOR_GREEN, COLOR_RESET);
        } else {
            printf("\n%s⚠️  WARNING: Performance may need optimization%s\n", 
                   COLOR_YELLOW, COLOR_RESET);
        }
        
    } else {
        printf("%s❌ No benchmarks completed successfully%s\n", COLOR_RED, COLOR_RESET);
    }
    
    /* Cleanup */
    hyperion_enhanced_errors_cleanup();
    hyperionConfigCleanup();
    hyperionMemoryCleanup();
    
    printf("\n%s=== Benchmark Complete ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    return successful_benchmarks > 0 ? 0 : 1;
}