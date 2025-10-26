#include "wasm_memory_manager.h"
#include "../../core/memory.h"
#include "../../utils/memory_pool.h"
#include <emscripten.h>
#include <stdlib.h>
#include <string.h>

// WASM-specific memory configuration
#define WASM_DEFAULT_HEAP_SIZE (16 * 1024 * 1024)  // 16MB default
#define WASM_MIN_POOL_SIZE (1024 * 1024)           // 1MB minimum
#define WASM_MAX_POOLS 8

// WASM memory manager state
static wasm_memory_manager_t g_wasm_manager = {0};
static bool g_initialized = false;

// JavaScript interop functions
EM_JS(void, js_memory_pressure_warning, (), {
    if (typeof Module.onMemoryPressure === 'function') {
        Module.onMemoryPressure();
    }
});

EM_JS(size_t, js_get_heap_size, (), {
    return Module.HEAP8.length;
});

EM_JS(bool, js_can_grow_memory, (size_t additional_bytes), {
    try {
        // Check if we can grow WebAssembly memory
        const pages_needed = Math.ceil(additional_bytes / 65536);
        return Module.wasmMemory.grow(pages_needed) !== -1;
    } catch (e) {
        return false;
    }
});

// Initialize WASM memory manager
int wasm_memory_manager_init(const wasm_memory_config_t* config) {
    if (g_initialized) {
        return WASM_MEMORY_SUCCESS;
    }
    
    memset(&g_wasm_manager, 0, sizeof(wasm_memory_manager_t));
    
    // Set configuration
    if (config) {
        g_wasm_manager.config = *config;
    } else {
        // Default configuration
        g_wasm_manager.config.heap_size = WASM_DEFAULT_HEAP_SIZE;
        g_wasm_manager.config.enable_gc = true;
        g_wasm_manager.config.pool_count = 4;
        g_wasm_manager.config.enable_monitoring = true;
    }
    
    // Initialize memory pools
    size_t pool_sizes[] = {1024, 4096, 16384, 65536};
    for (int i = 0; i < g_wasm_manager.config.pool_count && i < WASM_MAX_POOLS; i++) {
        size_t pool_size = (g_wasm_manager.config.heap_size / 8) / g_wasm_manager.config.pool_count;
        g_wasm_manager.pools[i] = memory_pool_create(pool_size);
        if (!g_wasm_manager.pools[i]) {
            // Cleanup on failure
            wasm_memory_manager_cleanup();
            return WASM_MEMORY_ERROR_INIT_FAILED;
        }
        g_wasm_manager.pool_block_sizes[i] = pool_sizes[i];
    }
    
    g_wasm_manager.total_allocated = 0;
    g_wasm_manager.peak_usage = 0;
    
    g_initialized = true;
    return WASM_MEMORY_SUCCESS;
}

// Allocate memory with WASM optimizations
void* wasm_memory_alloc(size_t size) {
    if (!g_initialized) {
        return malloc(size); // Fallback to standard malloc
    }
    
    if (size == 0) {
        return NULL;
    }
    
    // Find appropriate pool
    int pool_index = -1;
    for (int i = 0; i < g_wasm_manager.config.pool_count; i++) {
        if (size <= g_wasm_manager.pool_block_sizes[i]) {
            pool_index = i;
            break;
        }
    }
    
    void* ptr = NULL;
    
    if (pool_index >= 0 && g_wasm_manager.pools[pool_index]) {
        // Try pool allocation first
        ptr = memory_pool_alloc(g_wasm_manager.pools[pool_index], size);
    }
    
    if (!ptr) {
        // Fallback to malloc for large allocations or pool exhaustion
        ptr = malloc(size);
        
        if (!ptr) {
            // Try garbage collection and retry
            wasm_memory_gc();
            ptr = malloc(size);
        }
        
        if (!ptr) {
            // Try growing WASM memory
            if (js_can_grow_memory(size)) {
                ptr = malloc(size);
            }
        }
    }
    
    if (ptr) {
        g_wasm_manager.total_allocated += size;
        if (g_wasm_manager.total_allocated > g_wasm_manager.peak_usage) {
            g_wasm_manager.peak_usage = g_wasm_manager.total_allocated;
        }
        
        // Check for memory pressure
        size_t current_heap = js_get_heap_size();
        if (g_wasm_manager.total_allocated > (current_heap * 3 / 4)) {
            js_memory_pressure_warning();
        }
    }
    
    return ptr;
}

// Free memory with pool awareness
void wasm_memory_free(void* ptr, size_t size) {
    if (!ptr) {
        return;
    }
    
    if (!g_initialized) {
        free(ptr);
        return;
    }
    
    bool freed_from_pool = false;
    
    // Try to free from appropriate pool
    for (int i = 0; i < g_wasm_manager.config.pool_count; i++) {
        if (size <= g_wasm_manager.pool_block_sizes[i] && g_wasm_manager.pools[i]) {
            if (memory_pool_free(g_wasm_manager.pools[i], ptr) == 0) {
                freed_from_pool = true;
                break;
            }
        }
    }
    
    if (!freed_from_pool) {
        free(ptr);
    }
    
    g_wasm_manager.total_allocated -= size;
}

// Garbage collection for WASM
void wasm_memory_gc() {
    if (!g_initialized) {
        return;
    }
    
    // Compact memory pools
    for (int i = 0; i < g_wasm_manager.config.pool_count; i++) {
        if (g_wasm_manager.pools[i]) {
            memory_pool_defragment(g_wasm_manager.pools[i]);
        }
    }
    
    // Trigger JavaScript garbage collection if available
    EM_ASM({
        if (typeof gc === 'function') {
            gc();
        } else if (typeof Module.gc === 'function') {
            Module.gc();
        }
    });
}

// Get memory statistics
wasm_memory_stats_t wasm_memory_get_stats() {
    wasm_memory_stats_t stats = {0};
    
    if (!g_initialized) {
        return stats;
    }
    
    stats.total_allocated = g_wasm_manager.total_allocated;
    stats.peak_usage = g_wasm_manager.peak_usage;
    stats.heap_size = js_get_heap_size();
    
    // Calculate pool statistics
    for (int i = 0; i < g_wasm_manager.config.pool_count; i++) {
        if (g_wasm_manager.pools[i]) {
            memory_pool_stats_t pool_stats = memory_pool_get_stats(g_wasm_manager.pools[i]);
            stats.pool_allocated += pool_stats.allocated;
            stats.pool_free += pool_stats.free;
        }
    }
    
    stats.fragmentation_ratio = stats.pool_free > 0 ? 
        (float)stats.pool_allocated / (stats.pool_allocated + stats.pool_free) : 1.0f;
    
    return stats;
}

// Set memory limit
int wasm_memory_set_limit(size_t limit_bytes) {
    if (!g_initialized) {
        return WASM_MEMORY_ERROR_NOT_INITIALIZED;
    }
    
    g_wasm_manager.config.heap_size = limit_bytes;
    return WASM_MEMORY_SUCCESS;
}

// Check if memory allocation is possible
bool wasm_memory_can_allocate(size_t size) {
    if (!g_initialized) {
        return true; // Assume it's possible if not initialized
    }
    
    size_t current_heap = js_get_heap_size();
    return (g_wasm_manager.total_allocated + size) < current_heap;
}

// Cleanup memory manager
void wasm_memory_manager_cleanup() {
    if (!g_initialized) {
        return;
    }
    
    // Destroy all memory pools
    for (int i = 0; i < WASM_MAX_POOLS; i++) {
        if (g_wasm_manager.pools[i]) {
            memory_pool_destroy(g_wasm_manager.pools[i]);
            g_wasm_manager.pools[i] = NULL;
        }
    }
    
    memset(&g_wasm_manager, 0, sizeof(wasm_memory_manager_t));
    g_initialized = false;
}

// Export functions for JavaScript interop
EMSCRIPTEN_KEEPALIVE
void* wasm_malloc(size_t size) {
    return wasm_memory_alloc(size);
}

EMSCRIPTEN_KEEPALIVE
void wasm_free(void* ptr, size_t size) {
    wasm_memory_free(ptr, size);
}

EMSCRIPTEN_KEEPALIVE
wasm_memory_stats_t* wasm_get_memory_stats() {
    static wasm_memory_stats_t stats;
    stats = wasm_memory_get_stats();
    return &stats;
}

EMSCRIPTEN_KEEPALIVE
void wasm_trigger_gc() {
    wasm_memory_gc();
}