#ifndef TINYAI_BENCHMARK_UTILS_H
#define TINYAI_BENCHMARK_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Define benchmark result structure
typedef struct {
    // Timing metrics
    double total_time_ms;         // Total execution time in milliseconds
    double avg_inference_time_ms; // Average inference time per sample
    double std_dev_time_ms;       // Standard deviation of inference times
    double min_time_ms;           // Minimum inference time
    double max_time_ms;           // Maximum inference time

    // Memory metrics
    size_t peak_memory_bytes; // Peak memory usage in bytes
    size_t avg_memory_bytes;  // Average memory usage
    size_t model_size_bytes;  // Model size in bytes

    // Hardware utilization
    double cpu_utilization; // CPU utilization percentage
    int    threads_used;    // Number of threads used
    bool   simd_used;       // Whether SIMD was used
    char   simd_type[16];   // SIMD type (SSE2, AVX, AVX2, etc.)

    // Performance metrics
    int    samples_processed;  // Number of samples processed
    double samples_per_second; // Samples processed per second

    // Framework identification
    char framework_name[32];    // Name of the framework (Hyperion, TFLite, etc.)
    char framework_version[16]; // Version of the framework
    char model_name[64];        // Name of the model
    char device_name[64];       // Name of the device

    // Additional metrics specific to modality
    union {
        // Text-specific metrics
        struct {
            double tokens_per_second; // Tokens processed per second
            int    context_length;    // Context length used
        } text;

        // Image-specific metrics
        struct {
            int    image_width;  // Image width in pixels
            int    image_height; // Image height in pixels
            double fps;          // Frames per second
        } image;

        // Audio-specific metrics
        struct {
            int    sample_rate;      // Audio sample rate
            double audio_length_sec; // Audio length in seconds
            double real_time_factor; // Real-time processing factor (< 1 is faster than real-time)
        } audio;

        // Multimodal-specific metrics
        struct {
            int    num_modalities; // Number of modalities
            double fusion_time_ms; // Time for modality fusion
        } multimodal;
    } modality_metrics;
} HyperionBenchmarkResult;

// Define benchmark configuration structure
typedef struct {
    // General options
    bool use_simd;          // Whether to use SIMD acceleration
    int  num_threads;       // Number of threads to use (0 = auto)
    int  num_iterations;    // Number of iterations to run
    int  warmup_iterations; // Number of warmup iterations
    bool measure_memory;    // Whether to measure memory usage
    bool verbose;           // Whether to print verbose output

    // Model options
    char   model_path[256];    // Path to the model file
    bool   use_memory_mapping; // Whether to use memory-mapped model loading
    size_t cache_size_bytes;   // Cache size for memory-mapped loading

    // Export options
    bool export_csv;       // Whether to export results to CSV
    bool export_json;      // Whether to export results to JSON
    char export_path[256]; // Path for export files

    // Comparison options
    bool compare_frameworks;         // Whether to compare with other frameworks
    char comparison_frameworks[256]; // Comma-separated list of frameworks to compare
} HyperionBenchmarkConfig;

// Function to initialize benchmark configuration with defaults
void hyperion_init_benchmark_config(HyperionBenchmarkConfig *config);

// Function to initialize benchmark result structure
void hyperion_init_benchmark_result(HyperionBenchmarkResult *result);

// Function to start benchmark timing
void hyperion_benchmark_start_timer(struct timespec *start_time);

// Function to stop benchmark timing and calculate elapsed time in ms
double hyperion_benchmark_stop_timer(struct timespec *start_time);

// Function to export benchmark results to CSV
bool hyperion_export_benchmark_csv(const HyperionBenchmarkResult *result, const char *filepath);

// Function to export benchmark results to JSON
bool hyperion_export_benchmark_json(const HyperionBenchmarkResult *result, const char *filepath);

// Function to print benchmark results to console
void hyperion_print_benchmark_results(const HyperionBenchmarkResult *result);

// Function to compare benchmark results with other frameworks
void hyperion_compare_benchmark_results(const HyperionBenchmarkResult *hyperion_result,
                                      const HyperionBenchmarkResult *other_results,
                                      int                          num_other_results);

// Platform-specific memory measurement functions
size_t hyperion_measure_current_memory_usage(void);
size_t hyperion_measure_peak_memory_usage(void);

// Function to detect and return the available SIMD capabilities
const char *hyperion_detect_simd_capabilities(void);

// Function to determine the optimal number of threads
int hyperion_determine_optimal_threads(void);

// Utility function to create a timestamped filename
void hyperion_create_timestamped_filename(char *buffer, size_t buffer_size, const char *prefix,
                                        const char *extension);

#endif /* TINYAI_BENCHMARK_UTILS_H */
