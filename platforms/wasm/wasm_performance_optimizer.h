/**
 * WASM Performance Optimizer Header
 * 
 * Browser-specific performance optimizations for Hyperion WebAssembly
 */

#ifndef WASM_PERFORMANCE_OPTIMIZER_H
#define WASM_PERFORMANCE_OPTIMIZER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Performance recommendations structure */
typedef struct {
    int max_tokens;
    int memory_limit_mb;
    int use_streaming;
    int batch_size;
    int enable_caching;
} WasmPerfRecommendations;

/* Initialize performance optimization */
void wasm_perf_init(void);

/* Performance measurement */
double wasm_perf_start_timer(void);
void wasm_perf_end_timer(double start_time, int tokens_generated);

/* Browser optimization */
void wasm_perf_optimize_for_browser(void);
void wasm_perf_auto_tune(void);

/* Background processing */
void wasm_perf_schedule_background_task(void (*callback)(void));

/* Device-specific recommendations */
WasmPerfRecommendations wasm_perf_get_recommendations(void);

/* Statistics and monitoring */
void wasm_perf_print_stats(void);
void wasm_perf_reset_stats(void);

/* Cleanup */
void wasm_perf_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* WASM_PERFORMANCE_OPTIMIZER_H */
