/**
 * Hyperion Intermediate Example: File-Based Text Processor
 * 
 * This intermediate example demonstrates:
 * - Loading models and tokenizers from files
 * - Processing multiple text files in batch
 * - Basic error handling and validation
 * - Memory optimization techniques
 * - Configuration file usage
 * - Basic performance monitoring
 * 
 * Memory Usage: 50-150MB (depending on model)
 * Complexity: ⭐⭐⭐☆☆ (Intermediate)
 * 
 * What this example demonstrates:
 * - Real file I/O operations
 * - Batch processing optimization
 * - Error recovery strategies
 * - Configuration management
 * - Basic performance tracking
 * - Memory usage monitoring
 */

#include "../core/memory.h"
#include "../core/config.h"
#include "../core/io.h"
#include "../models/text/generate.h"
#include "../models/text/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Configuration structure */
typedef struct {
    char model_path[512];
    char tokenizer_path[512];
    char input_directory[512];
    char output_directory[512];
    int max_tokens;
    float temperature;
    int batch_size;
    int verbose;
} ProcessorConfig;

/* Statistics tracking */
typedef struct {
    int files_processed;
    int files_failed;
    int total_tokens_generated;
    double total_processing_time;
    size_t peak_memory_usage;
} ProcessingStats;

/* Load configuration from file */
int load_config(const char* config_file, ProcessorConfig* config) {
    printf("Loading configuration from %s...\n", config_file);
    
    /* Set defaults first */
    strcpy(config->model_path, "models/text_model.bin");
    strcpy(config->tokenizer_path, "models/tokenizer.txt");
    strcpy(config->input_directory, "input/");
    strcpy(config->output_directory, "output/");
    config->max_tokens = 200;
    config->temperature = 0.7f;
    config->batch_size = 5;
    config->verbose = 1;
    
    /* Try to load from file */
    HyperionFile* file = hyperionOpenFile(config_file, HYPERION_FILE_READ);
    if (!file) {
        printf("Config file not found, using defaults\n");
        return 0; /* Not an error, use defaults */
    }
    
    char line[512];
    int line_num = 0;
    
    while (hyperionReadLine(file, line, sizeof(line)) >= 0) {
        line_num++;
        
        /* Skip comments and empty lines */
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (*trimmed == '#' || *trimmed == '\0' || *trimmed == '\n') {
            continue;
        }
        
        /* Parse key=value pairs */
        char* equals = strchr(trimmed, '=');
        if (!equals) {
            printf("Warning: Invalid config line %d: %s\n", line_num, trimmed);
            continue;
        }
        
        *equals = '\0';
        char* key = trimmed;
        char* value = equals + 1;
        
        /* Remove trailing whitespace from key */
        char* key_end = key + strlen(key) - 1;
        while (key_end > key && (*key_end == ' ' || *key_end == '\t')) {
            *key_end-- = '\0';
        }
        
        /* Remove leading/trailing whitespace from value */
        while (*value == ' ' || *value == '\t') value++;
        char* value_end = value + strlen(value) - 1;
        while (value_end > value && (*value_end == ' ' || *value_end == '\t' || *value_end == '\n')) {
            *value_end-- = '\0';
        }
        
        /* Apply configuration */
        if (strcmp(key, "model_path") == 0) {
            strncpy(config->model_path, value, sizeof(config->model_path) - 1);
        } else if (strcmp(key, "tokenizer_path") == 0) {
            strncpy(config->tokenizer_path, value, sizeof(config->tokenizer_path) - 1);
        } else if (strcmp(key, "input_directory") == 0) {
            strncpy(config->input_directory, value, sizeof(config->input_directory) - 1);
        } else if (strcmp(key, "output_directory") == 0) {
            strncpy(config->output_directory, value, sizeof(config->output_directory) - 1);
        } else if (strcmp(key, "max_tokens") == 0) {
            config->max_tokens = atoi(value);
        } else if (strcmp(key, "temperature") == 0) {
            config->temperature = atof(value);
        } else if (strcmp(key, "batch_size") == 0) {
            config->batch_size = atoi(value);
        } else if (strcmp(key, "verbose") == 0) {
            config->verbose = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else {
            printf("Warning: Unknown config key '%s' on line %d\n", key, line_num);
        }
    }
    
    hyperionCloseFile(file);
    printf("✓ Configuration loaded successfully\n");
    return 0;
}

/* Validate configuration */
int validate_config(const ProcessorConfig* config) {
    printf("Validating configuration...\n");
    
    /* Check model file exists */
    if (!hyperionFileExists(config->model_path)) {
        fprintf(stderr, "Error: Model file not found: %s\n", config->model_path);
        return -1;
    }
    
    /* Check tokenizer file exists */
    if (!hyperionFileExists(config->tokenizer_path)) {
        fprintf(stderr, "Error: Tokenizer file not found: %s\n", config->tokenizer_path);
        return -1;
    }
    
    /* Validate numeric parameters */
    if (config->max_tokens <= 0 || config->max_tokens > 10000) {
        fprintf(stderr, "Error: max_tokens must be between 1 and 10000\n");
        return -1;
    }
    
    if (config->temperature < 0.0f || config->temperature > 2.0f) {
        fprintf(stderr, "Error: temperature must be between 0.0 and 2.0\n");
        return -1;
    }
    
    if (config->batch_size <= 0 || config->batch_size > 100) {
        fprintf(stderr, "Error: batch_size must be between 1 and 100\n");
        return -1;
    }
    
    printf("✓ Configuration validation passed\n");
    return 0;
}

/* Load model and tokenizer with error handling */
int load_model_and_tokenizer(const ProcessorConfig* config, 
                             HyperionModel** model, HyperionTokenizer** tokenizer) {
    printf("Loading model and tokenizer...\n");
    
    /* Create tokenizer */
    *tokenizer = hyperionCreateTokenizer();
    if (!*tokenizer) {
        fprintf(stderr, "Error: Failed to create tokenizer\n");
        return -1;
    }
    
    /* Load vocabulary */
    if (hyperionLoadVocabulary(*tokenizer, config->tokenizer_path) != 0) {
        fprintf(stderr, "Error: Failed to load tokenizer from %s\n", config->tokenizer_path);
        hyperionDestroyTokenizer(*tokenizer);
        *tokenizer = NULL;
        return -1;
    }
    
    int vocab_size = hyperionGetVocabularySize(*tokenizer);
    if (config->verbose) {
        printf("  Tokenizer loaded: %d vocabulary entries\n", vocab_size);
    }
    
    /* Create model */
    *model = hyperionCreateModel();
    if (!*model) {
        fprintf(stderr, "Error: Failed to create model\n");
        hyperionDestroyTokenizer(*tokenizer);
        *tokenizer = NULL;
        return -1;
    }
    
    /* Load model from file */
    if (hyperionLoadModelFromFile(*model, config->model_path) != 0) {
        fprintf(stderr, "Error: Failed to load model from %s\n", config->model_path);
        hyperionDestroyModel(*model);
        hyperionDestroyTokenizer(*tokenizer);
        *model = NULL;
        *tokenizer = NULL;
        return -1;
    }
    
    if (config->verbose) {
        printf("  Model loaded successfully\n");
    }
    
    /* Validate compatibility */
    if (hyperionValidateModelTokenizerCompatibility(*model, *tokenizer) != 0) {
        fprintf(stderr, "Error: Model and tokenizer are incompatible\n");
        hyperionDestroyModel(*model);
        hyperionDestroyTokenizer(*tokenizer);
        *model = NULL;
        *tokenizer = NULL;
        return -1;
    }
    
    printf("✓ Model and tokenizer loaded successfully\n");
    return 0;
}

/* Process a single text file */
int process_text_file(const char* input_file, const char* output_file,
                     HyperionModel* model, HyperionTokenizer* tokenizer,
                     const ProcessorConfig* config) {
    
    if (config->verbose) {
        printf("  Processing: %s -> %s\n", input_file, output_file);
    }
    
    /* Read input file */
    HyperionFile* input = hyperionOpenFile(input_file, HYPERION_FILE_READ);
    if (!input) {
        fprintf(stderr, "    Error: Cannot open input file %s\n", input_file);
        return -1;
    }
    
    /* Read entire input content */
    char input_text[4096];
    int64_t bytes_read = hyperionReadFile(input, input_text, sizeof(input_text) - 1);
    hyperionCloseFile(input);
    
    if (bytes_read <= 0) {
        fprintf(stderr, "    Error: Failed to read from %s\n", input_file);
        return -1;
    }
    
    input_text[bytes_read] = '\0';
    
    /* Remove trailing whitespace */
    while (bytes_read > 0 && (input_text[bytes_read-1] == '\n' || input_text[bytes_read-1] == '\r')) {
        input_text[--bytes_read] = '\0';
    }
    
    if (strlen(input_text) == 0) {
        fprintf(stderr, "    Warning: Input file %s is empty\n", input_file);
        return 0;
    }
    
    /* Tokenize input */
    int prompt_tokens[1024];
    int prompt_length = hyperionEncodeText(tokenizer, input_text, prompt_tokens, 1024);
    
    if (prompt_length <= 0) {
        fprintf(stderr, "    Error: Failed to tokenize input from %s\n", input_file);
        return -1;
    }
    
    /* Set up generation parameters */
    HyperionGenerationParams params = {0};
    params.maxTokens = config->max_tokens;
    params.temperature = config->temperature;
    params.samplingMethod = HYPERION_SAMPLING_TOP_P;
    params.topP = 0.9f;
    params.topK = 40;
    params.promptTokens = prompt_tokens;
    params.promptLength = prompt_length;
    params.seed = (unsigned int)time(NULL);
    
    /* Generate text */
    clock_t start_time = clock();
    
    int output_tokens[2048];
    int generated_length = hyperionGenerateText(model, &params, output_tokens, 2048);
    
    clock_t end_time = clock();
    double generation_time = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;
    
    if (generated_length <= 0) {
        fprintf(stderr, "    Error: Text generation failed for %s\n", input_file);
        return -1;
    }
    
    /* Decode output */
    char output_text[8192];
    if (hyperionDecodeTokens(tokenizer, output_tokens, generated_length, output_text, sizeof(output_text)) <= 0) {
        fprintf(stderr, "    Error: Failed to decode generated text for %s\n", input_file);
        return -1;
    }
    
    /* Write output file */
    HyperionFile* output = hyperionOpenFile(output_file, HYPERION_FILE_WRITE | HYPERION_FILE_CREATE | HYPERION_FILE_TRUNCATE);
    if (!output) {
        fprintf(stderr, "    Error: Cannot create output file %s\n", output_file);
        return -1;
    }
    
    /* Write original input as context */
    hyperionWriteFile(output, "=== INPUT ===\n", 14);
    hyperionWriteFile(output, input_text, strlen(input_text));
    hyperionWriteFile(output, "\n\n=== GENERATED ===\n", 19);
    hyperionWriteFile(output, output_text, strlen(output_text));
    hyperionWriteFile(output, "\n", 1);
    
    hyperionCloseFile(output);
    
    if (config->verbose) {
        printf("    ✓ Generated %d tokens in %.2f ms (%.2f tokens/sec)\n", 
               generated_length, generation_time, 
               generated_length / (generation_time / 1000.0));
    }
    
    return generated_length;
}

/* Get list of text files in directory */
int get_text_files(const char* directory, char files[][512], int max_files) {
    /* This is a simplified implementation - in a real system you'd use directory listing */
    /* For this example, we'll check for a few common filenames */
    
    const char* test_files[] = {
        "input1.txt", "input2.txt", "input3.txt", "test.txt", "sample.txt"
    };
    
    int file_count = 0;
    int num_test_files = sizeof(test_files) / sizeof(test_files[0]);
    
    for (int i = 0; i < num_test_files && file_count < max_files; i++) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s%s", directory, test_files[i]);
        
        if (hyperionFileExists(full_path)) {
            strcpy(files[file_count], test_files[i]);
            file_count++;
        }
    }
    
    return file_count;
}

int main(int argc, char *argv[]) {
    printf("=== Hyperion Intermediate File Processor ===\n");
    printf("Batch text processing with file I/O and optimization\n\n");
    
    /* Initialize Hyperion */
    if (hyperionConfigInit() != 0) {
        fprintf(stderr, "Error: Failed to initialize configuration\n");
        return 1;
    }
    
    if (hyperionIOInit() != 0) {
        fprintf(stderr, "Error: Failed to initialize I/O system\n");
        return 1;
    }
    
    hyperionMemTrackInit();
    
    /* Load configuration */
    ProcessorConfig config;
    const char* config_file = argc > 1 ? argv[1] : "processor.conf";
    
    if (load_config(config_file, &config) != 0) {
        fprintf(stderr, "Error: Failed to load configuration\n");
        goto cleanup;
    }
    
    /* Validate configuration */
    if (validate_config(&config) != 0) {
        fprintf(stderr, "Error: Configuration validation failed\n");
        goto cleanup;
    }
    
    /* Display configuration */
    printf("Configuration:\n");
    printf("  Model: %s\n", config.model_path);
    printf("  Tokenizer: %s\n", config.tokenizer_path);
    printf("  Input directory: %s\n", config.input_directory);
    printf("  Output directory: %s\n", config.output_directory);
    printf("  Max tokens: %d\n", config.max_tokens);
    printf("  Temperature: %.2f\n", config.temperature);
    printf("  Batch size: %d\n", config.batch_size);
    printf("  Verbose: %s\n\n", config.verbose ? "Yes" : "No");
    
    /* Load model and tokenizer */
    HyperionModel* model = NULL;
    HyperionTokenizer* tokenizer = NULL;
    
    if (load_model_and_tokenizer(&config, &model, &tokenizer) != 0) {
        goto cleanup;
    }
    
    /* Get list of input files */
    char input_files[100][512];
    int file_count = get_text_files(config.input_directory, input_files, 100);
    
    if (file_count == 0) {
        printf("No input files found in %s\n", config.input_directory);
        printf("Place some .txt files in the input directory and try again.\n");
        goto cleanup;
    }
    
    printf("Found %d input files to process\n\n", file_count);
    
    /* Initialize statistics */
    ProcessingStats stats = {0};
    clock_t total_start = clock();
    
    /* Process files in batches */
    for (int batch_start = 0; batch_start < file_count; batch_start += config.batch_size) {
        int batch_end = batch_start + config.batch_size;
        if (batch_end > file_count) batch_end = file_count;
        
        printf("Processing batch %d-%d...\n", batch_start + 1, batch_end);
        
        for (int i = batch_start; i < batch_end; i++) {
            char input_path[1024];
            char output_path[1024];
            
            snprintf(input_path, sizeof(input_path), "%s%s", 
                    config.input_directory, input_files[i]);
            snprintf(output_path, sizeof(output_path), "%sprocessed_%s", 
                    config.output_directory, input_files[i]);
            
            int tokens_generated = process_text_file(input_path, output_path, 
                                                   model, tokenizer, &config);
            
            if (tokens_generated > 0) {
                stats.files_processed++;
                stats.total_tokens_generated += tokens_generated;
            } else {
                stats.files_failed++;
            }
            
            /* Monitor memory usage */
            size_t current_memory = hyperionGetMemoryUsage();
            if (current_memory > stats.peak_memory_usage) {
                stats.peak_memory_usage = current_memory;
            }
        }
        
        /* Memory cleanup between batches */
        if (batch_end < file_count) {
            printf("  Performing batch cleanup...\n");
            /* Force garbage collection if available */
            /* hyperionGarbageCollect(); */
        }
        
        printf("  Batch completed\n\n");
    }
    
    clock_t total_end = clock();
    stats.total_processing_time = ((double)(total_end - total_start) / CLOCKS_PER_SEC) * 1000.0;
    
    /* Display final statistics */
    printf("=== Processing Complete ===\n");
    printf("Files processed: %d\n", stats.files_processed);
    printf("Files failed: %d\n", stats.files_failed);
    printf("Total tokens generated: %d\n", stats.total_tokens_generated);
    printf("Total processing time: %.2f seconds\n", stats.total_processing_time / 1000.0);
    printf("Average tokens per second: %.2f\n", 
           stats.total_tokens_generated / (stats.total_processing_time / 1000.0));
    printf("Peak memory usage: %.2f MB\n", stats.peak_memory_usage / (1024.0 * 1024.0));
    
    if (stats.files_processed > 0) {
        printf("Average processing time per file: %.2f seconds\n", 
               (stats.total_processing_time / 1000.0) / stats.files_processed);
        printf("Average tokens per file: %.2f\n", 
               (double)stats.total_tokens_generated / stats.files_processed);
    }
    
    printf("\nOutput files saved to: %s\n", config.output_directory);
    
cleanup:
    printf("\nCleaning up...\n");
    
    if (model) {
        hyperionDestroyModel(model);
    }
    
    if (tokenizer) {
        hyperionDestroyTokenizer(tokenizer);
    }
    
    /* Check for memory leaks */
    int leak_count = hyperionMemTrackDumpLeaks();
    if (leak_count == 0) {
        printf("✓ No memory leaks detected\n");
    } else {
        printf("⚠ %d memory leaks detected\n", leak_count);
    }
    
    hyperionMemTrackCleanup();
    hyperionIOCleanup();
    hyperionConfigCleanup();
    
    printf("✓ Cleanup completed\n");
    
    return 0;
}

/*
 * Build Instructions:
 * 
 * gcc -o intermediate_file_processor intermediate_file_processor.c \
 *     ../core/memory.c ../core/config.c ../core/io.c \
 *     ../models/text/generate.c ../models/text/tokenizer.c \
 *     -I.. -lm
 * 
 * Create sample configuration file (processor.conf):
 * model_path=models/text_model.bin
 * tokenizer_path=models/tokenizer.txt
 * input_directory=input/
 * output_directory=output/
 * max_tokens=200
 * temperature=0.7
 * batch_size=5
 * verbose=true
 * 
 * Usage:
 * mkdir -p input output
 * echo "Hello world" > input/test.txt
 * ./intermediate_file_processor [config_file]
 * 
 * Features demonstrated:
 * - Configuration file parsing
 * - File I/O operations with error handling
 * - Batch processing optimization
 * - Memory usage monitoring
 * - Performance statistics tracking
 * - Input validation and error recovery
 */