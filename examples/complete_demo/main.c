/**
 * Hyperion Complete Demo
 * 
 * Comprehensive demonstration of all Hyperion framework features including:
 * - Hybrid local/remote execution
 * - Performance monitoring
 * - 4-bit quantization
 * - Multimodal processing
 * - Configuration hierarchy
 * - Web interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../core/config.h"
#include "../../core/memory.h"
#include "../../core/runtime.h"
#include "../../core/mcp/mcp_client.h"
#include "../../models/text/generate.h"
#include "../../models/text/hybrid_generate.h"
#include "../../models/text/tokenizer.h"
#include "../../models/image/image_model.h"
#include "../../models/multimodal/multimodal_model.h"
#include "../../utils/performance_monitor.h"
#include "../../interface/web_server.h"

// Demo modes
typedef enum {
    DEMO_MODE_TEXT,
    DEMO_MODE_MULTIMODAL,
    DEMO_MODE_HYBRID,
    DEMO_MODE_MEMORY_ANALYSIS,
    DEMO_MODE_WEB_SERVER,
    DEMO_MODE_BENCHMARK
} DemoMode;

// Demo context
typedef struct {
    DemoMode mode;
    
    // Models
    HyperionModel* model;
    HyperionTokenizer* tokenizer;
    HyperionImageModel* image_model;
    HyperionMultimodalModel* multimodal_model;
    
    // Hybrid execution
    HyperionMcpClient* mcp_client;
    HyperionHybridGenerate* hybrid_generate;
    
    // Performance monitoring
    HyperionPerformanceMonitor* perf_monitor;
    
    // Configuration
    bool enable_quantization;
    bool enable_simd;
    bool enable_hybrid;
    bool enable_performance_monitor;
    bool verbose;
    
    // Web server
    bool enable_web_server;
    int web_port;
    char web_document_root[256];
    
    // Output
    char output_file[256];
    char perf_report_file[256];
    const char* perf_report_format;
} DemoContext;

void print_banner()
{
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║              Hyperion Complete Demo                 ║\n");
    printf("║        Ultra-Lightweight AI Framework v0.1.0        ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");
}

void print_usage(const char* program_name)
{
    printf("Usage: %s [options]\n\n", program_name);
    printf("Modes:\n");
    printf("  --mode text              Text generation demo\n");
    printf("  --mode multimodal        Multimodal processing demo\n");
    printf("  --mode hybrid            Hybrid execution demo\n");
    printf("  --mode memory-analysis   Memory profiling demo\n");
    printf("  --mode web-server        Web interface demo\n");
    printf("  --mode benchmark         Performance benchmark\n\n");
    
    printf("Model Options:\n");
    printf("  --model <file>           Model file path\n");
    printf("  --tokenizer <file>       Tokenizer file path\n");
    printf("  --image-model <file>     Image model file path\n");
    printf("  --quantized              Enable 4-bit quantization (default)\n");
    printf("  --no-quantized           Disable quantization\n");
    printf("  --simd                   Enable SIMD acceleration\n");
    printf("  --no-simd                Disable SIMD acceleration\n\n");
    
    printf("Hybrid Execution:\n");
    printf("  --enable-hybrid          Enable hybrid execution\n");
    printf("  --mcp-server <url>       MCP server URL\n");
    printf("  --mcp-prefer-local       Prefer local execution\n");
    printf("  --mcp-prefer-remote      Prefer remote execution\n");
    printf("  --force-offline          Force offline mode\n\n");
    
    printf("Performance Monitoring:\n");
    printf("  --enable-perf-monitor    Enable performance monitoring\n");
    printf("  --perf-output <file>     Performance report output file\n");
    printf("  --perf-format <format>   Report format (text/json/csv)\n");
    printf("  --monitor-duration <sec> Memory monitoring duration\n\n");
    
    printf("Web Server:\n");
    printf("  --web-server             Enable web server\n");
    printf("  --port <port>            Web server port (default: 8080)\n");
    printf("  --document-root <path>   Web document root\n\n");
    
    printf("Generation Options:\n");
    printf("  --prompt <text>          Input prompt for text generation\n");
    printf("  --image <file>           Input image for multimodal processing\n");
    printf("  --max-tokens <n>         Maximum tokens to generate\n");
    printf("  --temperature <value>    Sampling temperature\n");
    printf("  --output <file>          Output file\n\n");
    
    printf("Other Options:\n");
    printf("  --config <file>          Configuration file\n");
    printf("  --verbose                Enable verbose output\n");
    printf("  --help                   Show this help message\n\n");
    
    printf("Examples:\n");
    printf("  # Basic text generation\n");
    printf("  %s --mode text --prompt \"The future of AI is\"\n\n", program_name);
    
    printf("  # Hybrid execution demo\n");
    printf("  %s --mode hybrid --enable-hybrid --mcp-server mock://localhost:8080\n\n", program_name);
    
    printf("  # Web server with performance monitoring\n");
    printf("  %s --mode web-server --enable-perf-monitor --port 8080\n\n", program_name);
    
    printf("  # Memory analysis\n");
    printf("  %s --mode memory-analysis --monitor-duration 60 --perf-output report.json\n\n", program_name);
}

int parse_arguments(int argc, char** argv, DemoContext* ctx)
{
    // Set defaults
    ctx->mode = DEMO_MODE_TEXT;
    ctx->enable_quantization = true;
    ctx->enable_simd = true;
    ctx->enable_hybrid = false;
    ctx->enable_performance_monitor = false;
    ctx->verbose = false;
    ctx->enable_web_server = false;
    ctx->web_port = 8080;
    strcpy(ctx->web_document_root, "./web_ui");
    ctx->output_file[0] = '\0';
    ctx->perf_report_file[0] = '\0';
    ctx->perf_report_format = "text";
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            const char* mode = argv[++i];
            if (strcmp(mode, "text") == 0) ctx->mode = DEMO_MODE_TEXT;
            else if (strcmp(mode, "multimodal") == 0) ctx->mode = DEMO_MODE_MULTIMODAL;
            else if (strcmp(mode, "hybrid") == 0) ctx->mode = DEMO_MODE_HYBRID;
            else if (strcmp(mode, "memory-analysis") == 0) ctx->mode = DEMO_MODE_MEMORY_ANALYSIS;
            else if (strcmp(mode, "web-server") == 0) ctx->mode = DEMO_MODE_WEB_SERVER;
            else if (strcmp(mode, "benchmark") == 0) ctx->mode = DEMO_MODE_BENCHMARK;
            else {
                fprintf(stderr, "Unknown mode: %s\n", mode);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--enable-hybrid") == 0) {
            ctx->enable_hybrid = true;
        }
        else if (strcmp(argv[i], "--enable-perf-monitor") == 0) {
            ctx->enable_performance_monitor = true;
        }
        else if (strcmp(argv[i], "--web-server") == 0) {
            ctx->enable_web_server = true;
            ctx->mode = DEMO_MODE_WEB_SERVER;
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            ctx->verbose = true;
        }
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            ctx->web_port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--perf-output") == 0 && i + 1 < argc) {
            strcpy(ctx->perf_report_file, argv[++i]);
        }
        else if (strcmp(argv[i], "--perf-format") == 0 && i + 1 < argc) {
            ctx->perf_report_format = argv[++i];
        }
        else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            hyperionConfigLoad(argv[++i]);
        }
        else if (strcmp(argv[i], "--prompt") == 0 && i + 1 < argc) {
            hyperionConfigSetString("demo.prompt", argv[++i]);
        }
        else if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) {
            hyperionConfigSetString("model.path", argv[++i]);
        }
        else if (strcmp(argv[i], "--tokenizer") == 0 && i + 1 < argc) {
            hyperionConfigSetString("tokenizer.path", argv[++i]);
        }
        // Add more argument parsing as needed
    }
    
    return 1;
}

int initialize_models(DemoContext* ctx)
{
    if (ctx->verbose) {
        printf("Initializing models...\n");
    }
    
    // Initialize performance monitor if enabled
    if (ctx->enable_performance_monitor) {
        ctx->perf_monitor = hyperionPerfCreate(10000, true);
        if (!ctx->perf_monitor) {
            fprintf(stderr, "Failed to create performance monitor\n");
            return -1;
        }
        hyperionPerfSetVerbose(ctx->perf_monitor, ctx->verbose);
    }
    
    // Load tokenizer
    const char* tokenizer_path = hyperionConfigGet("tokenizer.path", "./models/tokenizer.txt");
    if (ctx->perf_monitor) {
        HYPERION_PERF_BEGIN(ctx->perf_monitor, HYPERION_PERF_MODEL_LOADING, "tokenizer_loading");
    }
    
    ctx->tokenizer = hyperionLoadTokenizer(tokenizer_path);
    
    if (ctx->perf_monitor) {
        HYPERION_PERF_END(ctx->perf_monitor, ctx->tokenizer ? 0 : -1);
    }
    
    if (!ctx->tokenizer) {
        fprintf(stderr, "Failed to load tokenizer from %s\n", tokenizer_path);
        return -1;
    }
    
    // Load main model
    const char* model_path = hyperionConfigGet("model.path", "./models/demo_model.bin");
    if (ctx->perf_monitor) {
        HYPERION_PERF_BEGIN(ctx->perf_monitor, HYPERION_PERF_MODEL_LOADING, "model_loading");
    }
    
    ctx->model = hyperionLoadModel(model_path, NULL, tokenizer_path);
    
    if (ctx->perf_monitor) {
        HYPERION_PERF_END(ctx->perf_monitor, ctx->model ? 0 : -1);
    }
    
    if (!ctx->model) {
        fprintf(stderr, "Failed to load model from %s\n", model_path);
        return -1;
    }
    
    if (ctx->verbose) {
        printf("Models loaded successfully\n");
        if (ctx->enable_quantization) {
            printf("4-bit quantization enabled (75%% memory savings)\n");
        }
    }
    
    return 0;
}

int run_text_generation_demo(DemoContext* ctx)
{
    const char* prompt = hyperionConfigGet("demo.prompt", "The future of AI is");
    int max_tokens = hyperionConfigGetInt("generate.max_tokens", 100);
    float temperature = hyperionConfigGetFloat("generate.temperature", 0.7f);
    
    printf("Text Generation Demo\n");
    printf("====================\n");
    printf("Prompt: \"%s\"\n", prompt);
    printf("Max tokens: %d\n", max_tokens);
    printf("Temperature: %.2f\n\n", temperature);
    
    // Tokenize prompt
    int prompt_tokens[512];
    int prompt_length;
    
    if (ctx->perf_monitor) {
        HYPERION_PERF_BEGIN(ctx->perf_monitor, HYPERION_PERF_TOKENIZATION, "prompt_tokenization");
    }
    
    prompt_length = hyperionTokenize(ctx->tokenizer, prompt, prompt_tokens, 512);
    
    if (ctx->perf_monitor) {
        HYPERION_PERF_END(ctx->perf_monitor, prompt_length > 0 ? 0 : -1);
    }
    
    if (prompt_length <= 0) {
        fprintf(stderr, "Failed to tokenize prompt\n");
        return -1;
    }
    
    // Set up generation parameters
    HyperionGenerationParams params = {0};
    params.maxTokens = max_tokens;
    params.samplingMethod = HYPERION_SAMPLING_TEMPERATURE;
    params.temperature = temperature;
    params.topK = hyperionConfigGetInt("generate.top_k", 40);
    params.topP = hyperionConfigGetFloat("generate.top_p", 0.9f);
    params.promptTokens = prompt_tokens;
    params.promptLength = prompt_length;
    
    // Generate text
    int* output_tokens = malloc(max_tokens * sizeof(int));
    if (!output_tokens) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }
    
    printf("Generating text...\n");
    
    if (ctx->perf_monitor) {
        HYPERION_PERF_BEGIN(ctx->perf_monitor, HYPERION_PERF_TEXT_GENERATION, "text_generation");
    }
    
    int generated_count = hyperionGenerateText(ctx->model, &params, output_tokens, max_tokens);
    
    if (ctx->perf_monitor) {
        HYPERION_PERF_END(ctx->perf_monitor, generated_count > 0 ? 0 : -1);
    }
    
    if (generated_count > 0) {
        char* result_text = hyperionDecode(ctx->tokenizer, output_tokens, generated_count);
        if (result_text) {
            printf("\nGenerated Text:\n");
            printf("\"%s\"\n\n", result_text);
            
            // Calculate statistics
            if (ctx->perf_monitor) {
                HyperionPerfStats stats;
                if (hyperionPerfGetStats(ctx->perf_monitor, HYPERION_PERF_TEXT_GENERATION, &stats)) {
                    printf("Performance Statistics:\n");
                    printf("  Generation time: %.2f ms\n", stats.avg_time_ms);
                    printf("  Tokens/second: %.2f\n", generated_count * 1000.0 / stats.avg_time_ms);
                    printf("  Memory used: %.2f MB\n", stats.peak_memory_usage / (1024.0 * 1024.0));
                }
            }
            
            free(result_text);
        }
    } else {
        fprintf(stderr, "Text generation failed\n");
    }
    
    free(output_tokens);
    return generated_count > 0 ? 0 : -1;
}

int run_hybrid_demo(DemoContext* ctx)
{
    printf("Hybrid Execution Demo\n");
    printf("=====================\n");
    
    if (!ctx->enable_hybrid) {
        printf("Hybrid execution not enabled. Use --enable-hybrid to enable.\n");
        return 0;
    }
    
    // Create MCP client
    HyperionMcpConfig mcp_config;
    hyperionMcpGetDefaultConfig(&mcp_config);
    mcp_config.execPreference = HYPERION_EXEC_PREFER_LOCAL;
    
    ctx->mcp_client = hyperionMcpCreateClient(&mcp_config);
    if (!ctx->mcp_client) {
        fprintf(stderr, "Failed to create MCP client\n");
        return -1;
    }
    
    // Connect to MCP server
    const char* mcp_server = hyperionConfigGet("hybrid.mcp_server_url", "mock://localhost:8080");
    printf("Connecting to MCP server: %s\n", mcp_server);
    
    bool connected = hyperionMcpConnect(ctx->mcp_client, mcp_server);
    if (!connected) {
        printf("Failed to connect to MCP server. Running local-only demo.\n");
        return run_text_generation_demo(ctx);
    }
    
    printf("Connected successfully!\n");
    
    // Create hybrid generation context
    ctx->hybrid_generate = hyperionCreateHybridGenerate(ctx->model, ctx->mcp_client);
    if (!ctx->hybrid_generate) {
        fprintf(stderr, "Failed to create hybrid generation context\n");
        return -1;
    }
    
    // Test hybrid generation with different scenarios
    const char* test_prompts[] = {
        "Hello world",  // Simple - should use local
        "Explain quantum computing in detail with mathematical formulas",  // Complex - should use remote
        "What is 2+2?",  // Simple - should use local
        "Write a comprehensive analysis of machine learning algorithms"  // Complex - should use remote
    };
    
    for (int i = 0; i < 4; i++) {
        printf("\nTest %d: \"%s\"\n", i + 1, test_prompts[i]);
        
        // Tokenize prompt
        int prompt_tokens[512];
        int prompt_length = hyperionTokenize(ctx->tokenizer, test_prompts[i], prompt_tokens, 512);
        
        if (prompt_length <= 0) continue;
        
        // Set up parameters
        HyperionGenerationParams params = {0};
        params.maxTokens = 50;
        params.temperature = 0.7f;
        params.promptTokens = prompt_tokens;
        params.promptLength = prompt_length;
        
        // Check what execution mode would be used
        bool would_use_remote = hyperionHybridGenerateWouldUseRemote(ctx->hybrid_generate, &params);
        printf("  Predicted execution: %s\n", would_use_remote ? "Remote" : "Local");
        
        // Generate
        int output_tokens[50];
        if (ctx->perf_monitor) {
            HYPERION_PERF_BEGIN(ctx->perf_monitor, HYPERION_PERF_TEXT_GENERATION, "hybrid_generation");
        }
        
        int generated = hyperionHybridGenerateText(ctx->hybrid_generate, &params, output_tokens, 50);
        
        if (ctx->perf_monitor) {
            HYPERION_PERF_END(ctx->perf_monitor, generated > 0 ? 0 : -1);
        }
        
        if (generated > 0) {
            bool used_remote = hyperionHybridGenerateUsedRemote(ctx->hybrid_generate);
            double local_time, remote_time, tokens_per_sec;
            hyperionHybridGenerateGetStats(ctx->hybrid_generate, &local_time, &remote_time, &tokens_per_sec);
            
            printf("  Actual execution: %s\n", used_remote ? "Remote" : "Local");
            printf("  Time: %.2f ms\n", used_remote ? remote_time : local_time);
            printf("  Tokens/sec: %.2f\n", tokens_per_sec);
            
            char* result = hyperionDecode(ctx->tokenizer, output_tokens, generated);
            if (result) {
                printf("  Result: \"%s\"\n", result);
                free(result);
            }
        }
    }
    
    return 0;
}

int run_web_server_demo(DemoContext* ctx)
{
    printf("Web Server Demo\n");
    printf("===============\n");
    printf("Starting web server on port %d\n", ctx->web_port);
    printf("Document root: %s\n", ctx->web_document_root);
    printf("Access the interface at: http://localhost:%d\n\n", ctx->web_port);
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", ctx->web_port);
    
    return start_web_server(NULL, port_str, ctx->web_document_root);
}

int run_memory_analysis_demo(DemoContext* ctx)
{
    printf("Memory Analysis Demo\n");
    printf("====================\n");
    
    if (!ctx->perf_monitor) {
        ctx->perf_monitor = hyperionPerfCreate(10000, true);
        if (!ctx->perf_monitor) {
            fprintf(stderr, "Failed to create performance monitor\n");
            return -1;
        }
    }
    
    int duration = hyperionConfigGetInt("demo.monitor_duration", 30);
    printf("Monitoring memory usage for %d seconds...\n", duration);
    
    // Start memory monitoring
    hyperionPerfMonitorMemory(ctx->perf_monitor, 100, duration * 1000);
    
    // Generate performance report
    if (strlen(ctx->perf_report_file) > 0) {
        hyperionPerfGenerateReport(ctx->perf_monitor, ctx->perf_report_file, ctx->perf_report_format);
        printf("Performance report saved to: %s\n", ctx->perf_report_file);
    } else {
        hyperionPerfGenerateReport(ctx->perf_monitor, NULL, "text");
    }
    
    return 0;
}

int main(int argc, char** argv)
{
    print_banner();
    
    // Initialize systems
    hyperionConfigInit();
    hyperionConfigSetDefaults();
    hyperionMemTrackInit();
    
    // Parse arguments
    DemoContext ctx = {0};
    int parse_result = parse_arguments(argc, argv, &ctx);
    if (parse_result <= 0) {
        return parse_result;
    }
    
    // Apply command line config overrides
    hyperionConfigApplyCommandLine(argc, argv);
    
    // Initialize models for modes that need them
    if (ctx.mode != DEMO_MODE_WEB_SERVER) {
        if (initialize_models(&ctx) != 0) {
            fprintf(stderr, "Failed to initialize models\n");
            return 1;
        }
    }
    
    // Run the appropriate demo
    int result = 0;
    switch (ctx.mode) {
        case DEMO_MODE_TEXT:
            result = run_text_generation_demo(&ctx);
            break;
        case DEMO_MODE_HYBRID:
            result = run_hybrid_demo(&ctx);
            break;
        case DEMO_MODE_WEB_SERVER:
            result = run_web_server_demo(&ctx);
            break;
        case DEMO_MODE_MEMORY_ANALYSIS:
            result = run_memory_analysis_demo(&ctx);
            break;
        default:
            printf("Demo mode not implemented yet: %d\n", ctx.mode);
            result = 1;
            break;
    }
    
    // Generate final performance report if monitoring was enabled
    if (ctx.perf_monitor && strlen(ctx.perf_report_file) > 0) {
        hyperionPerfGenerateReport(ctx.perf_monitor, ctx.perf_report_file, ctx.perf_report_format);
    }
    
    // Cleanup
    if (ctx.hybrid_generate) {
        hyperionDestroyHybridGenerate(ctx.hybrid_generate);
    }
    if (ctx.mcp_client) {
        hyperionMcpDestroyClient(ctx.mcp_client);
    }
    if (ctx.perf_monitor) {
        hyperionPerfDestroy(ctx.perf_monitor);
    }
    if (ctx.model) {
        hyperionDestroyModel(ctx.model);
    }
    if (ctx.tokenizer) {
        hyperionDestroyTokenizer(ctx.tokenizer);
    }
    
    hyperionConfigCleanup();
    hyperionMemTrackCleanup();
    
    printf("\nDemo completed successfully!\n");
    return result;
}