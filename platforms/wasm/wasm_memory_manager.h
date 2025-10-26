/**
 * WASM Memory Manager Header
 * 
 * Specialized memory management for WebAssembly environment
 */

#ifndef WASM_MEMORY_MANAGER_H
#define WASM_MEMORY_MANAGER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize WASM memory management */
int wasm_memory_init(size_t memory_limit);

/* WASM-optimized memory allocation */
void* wasm_malloc(size_t size);
void wasm_free(void* ptr);

/* Memory statistics */
size_t wasm_get_memory_usage(void);
size_t wasm_get_peak_memory_usage(void);
int wasm_get_allocation_count(void);

/* Memory optimization */
void wasm_force_gc(void);
void wasm_optimize_memory(void);
int wasm_can_allocate(size_t size);
void wasm_set_memory_limit(size_t new_limit);

/* Debugging and monitoring */
void wasm_print_memory_stats(void);
int wasm_check_memory_leaks(void);

/* Cleanup */
void wasm_memory_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* WASM_MEMORY_MANAGER_H */