/**
 * WASM Memory Manager
 * 
 * Specialized memory management for WebAssembly environment
 * Optimized for browser memory constraints and garbage collection
 */

#include "wasm_memory_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

/* WASM memory tracking */
typedef struct WasmMemoryBlock {
    void* ptr;
    size_t size;
    struct WasmMemoryBlock* next;
} WasmMemoryBlock;

typedef struct {
    WasmMemoryBlock* blocks;
    size_t total_allocated;
    size_t peak_usage;
    size_t memory_limit;
    int allocation_count;
    int initialized;
} WasmMemoryTracker;

static WasmMemoryTracker g_wasm_memory = {0};

/**
 * Initialize WASM memory management
 */
int wasm_memory_init(size_t memory_limit) {
    if (g_wasm_memory.initialized) {
        printf("WASM memory manager already initialized\n");
        return 0;
    }
    
    printf("Initializing WASM memory manager with limit: %.2f MB\n", 
           memory_limit / (1024.0 * 1024.0));
    
    g_wasm_memory.memory_limit = memory_limit;
    g_wasm_memory.blocks = NULL;
    g_wasm_memory.total_allocated = 0;
    g_wasm_memory.peak_usage = 0;
    g_wasm_memory.allocation_count = 0;
    g_wasm_memory.initialized = 1;
    
    return 0;
}

/**
 * WASM-optimized allocation with tracking
 */
void* wasm_malloc(size_t size) {
    if (!g_wasm_memory.initialized) {
        printf("WASM memory manager not initialized, using standard malloc\n");
        return malloc(size);
    }
    
    // Check memory limit
    if (g_wasm_memory.total_allocated + size > g_wasm_memory.memory_limit) {
        printf("WASM memory limit exceeded: requested %zu bytes, "
               "current usage %zu bytes, limit %zu bytes\n",
               size, g_wasm_memory.total_allocated, g_wasm_memory.memory_limit);
        
        // Try garbage collection hint to browser
        emscripten_run_script("if (window.gc) window.gc();");
        
        // Still check limit after GC hint
        if (g_wasm_memory.total_allocated + size > g_wasm_memory.memory_limit) {
            return NULL;
        }
    }
    
    // Allocate memory
    void* ptr = malloc(size);
    if (!ptr) {
        printf("WASM allocation failed for %zu bytes\n", size);
        return NULL;
    }
    
    // Track allocation
    WasmMemoryBlock* block = (WasmMemoryBlock*)malloc(sizeof(WasmMemoryBlock));
    if (block) {
        block->ptr = ptr;
        block->size = size;
        block->next = g_wasm_memory.blocks;
        g_wasm_memory.blocks = block;
        
        g_wasm_memory.total_allocated += size;
        g_wasm_memory.allocation_count++;
        
        if (g_wasm_memory.total_allocated > g_wasm_memory.peak_usage) {
            g_wasm_memory.peak_usage = g_wasm_memory.total_allocated;
        }
        
        // Debug output for large allocations
        if (size > 1024 * 1024) {
            printf("WASM large allocation: %.2f MB, total: %.2f MB\n",
                   size / (1024.0 * 1024.0),
                   g_wasm_memory.total_allocated / (1024.0 * 1024.0));
        }
    }
    
    return ptr;
}

/**
 * WASM-optimized free with tracking
 */
void wasm_free(void* ptr) {
    if (!ptr) return;
    
    if (!g_wasm_memory.initialized) {
        free(ptr);
        return;
    }
    
    // Find and remove from tracking
    WasmMemoryBlock* prev = NULL;
    WasmMemoryBlock* current = g_wasm_memory.blocks;
    
    while (current) {
        if (current->ptr == ptr) {
            // Remove from list
            if (prev) {
                prev->next = current->next;
            } else {
                g_wasm_memory.blocks = current->next;
            }
            
            // Update counters
            g_wasm_memory.total_allocated -= current->size;
            g_wasm_memory.allocation_count--;
            
            // Free tracking block
            free(current);
            break;
        }
        
        prev = current;
        current = current->next;
    }
    
    // Free actual memory
    free(ptr);
}

/**
 * Get current memory usage
 */
size_t wasm_get_memory_usage(void) {
    return g_wasm_memory.initialized ? g_wasm_memory.total_allocated : 0;
}

/**
 * Get peak memory usage
 */
size_t wasm_get_peak_memory_usage(void) {
    return g_wasm_memory.initialized ? g_wasm_memory.peak_usage : 0;
}

/**
 * Get allocation count
 */
int wasm_get_allocation_count(void) {
    return g_wasm_memory.initialized ? g_wasm_memory.allocation_count : 0;
}

/**
 * Force garbage collection hint
 */
void wasm_force_gc(void) {
    printf("Suggesting garbage collection to browser...\n");
    
    // Hint to browser to run garbage collection
    EM_ASM({
        if (typeof window !== 'undefined' && window.gc) {
            window.gc();
            console.log('Manual GC triggered');
        } else if (typeof global !== 'undefined' && global.gc) {
            global.gc();
            console.log('Manual GC triggered (Node.js)');
        }
    });
}

/**
 * Optimize memory layout for better performance
 */
void wasm_optimize_memory(void) {
    printf("Optimizing WASM memory layout...\n");
    
    // Defragmentation hint (browser-specific)
    EM_ASM({
        // Request memory compaction if available
        if (typeof WebAssembly !== 'undefined' && WebAssembly.Memory && 
            WebAssembly.Memory.prototype.grow) {
            console.log('Memory optimization requested');
        }
    });
    
    wasm_force_gc();
}

/**
 * Check if memory allocation would succeed
 */
int wasm_can_allocate(size_t size) {
    if (!g_wasm_memory.initialized) {
        return 1; // Assume success if not tracking
    }
    
    return (g_wasm_memory.total_allocated + size <= g_wasm_memory.memory_limit) ? 1 : 0;
}

/**
 * Set new memory limit
 */
void wasm_set_memory_limit(size_t new_limit) {
    if (g_wasm_memory.initialized) {
        printf("Updating WASM memory limit from %.2f MB to %.2f MB\n",
               g_wasm_memory.memory_limit / (1024.0 * 1024.0),
               new_limit / (1024.0 * 1024.0));
        g_wasm_memory.memory_limit = new_limit;
    }
}

/**
 * Print memory statistics
 */
void wasm_print_memory_stats(void) {
    if (!g_wasm_memory.initialized) {
        printf("WASM memory manager not initialized\n");
        return;
    }
    
    printf("=== WASM Memory Statistics ===\n");
    printf("Current usage: %.2f MB\n", g_wasm_memory.total_allocated / (1024.0 * 1024.0));
    printf("Peak usage: %.2f MB\n", g_wasm_memory.peak_usage / (1024.0 * 1024.0));
    printf("Memory limit: %.2f MB\n", g_wasm_memory.memory_limit / (1024.0 * 1024.0));
    printf("Utilization: %.1f%%\n", 
           100.0 * g_wasm_memory.total_allocated / g_wasm_memory.memory_limit);
    printf("Active allocations: %d\n", g_wasm_memory.allocation_count);
    
    // Check for potential leaks
    if (g_wasm_memory.allocation_count > 1000) {
        printf("WARNING: High allocation count may indicate memory leaks\n");
    }
    
    // Browser-specific memory info
    EM_ASM({
        if (typeof performance !== 'undefined' && performance.memory) {
            console.log('Browser memory stats:');
            console.log('  Used JS heap:', (performance.memory.usedJSHeapSize / 1024 / 1024).toFixed(2), 'MB');
            console.log('  Total JS heap:', (performance.memory.totalJSHeapSize / 1024 / 1024).toFixed(2), 'MB');
            console.log('  Heap limit:', (performance.memory.jsHeapSizeLimit / 1024 / 1024).toFixed(2), 'MB');
        }
    });
}

/**
 * Check for memory leaks
 */
int wasm_check_memory_leaks(void) {
    if (!g_wasm_memory.initialized) {
        return 0;
    }
    
    int leak_count = 0;
    WasmMemoryBlock* current = g_wasm_memory.blocks;
    
    printf("Checking for memory leaks...\n");
    
    while (current) {
        leak_count++;
        if (leak_count <= 10) { // Show first 10 leaks
            printf("  Leak: %zu bytes at %p\n", current->size, current->ptr);
        }
        current = current->next;
    }
    
    if (leak_count > 0) {
        printf("Found %d potential memory leaks\n", leak_count);
        if (leak_count > 10) {
            printf("  (showing first 10 only)\n");
        }
    } else {
        printf("No memory leaks detected\n");
    }
    
    return leak_count;
}

/**
 * Cleanup WASM memory manager
 */
void wasm_memory_cleanup(void) {
    if (!g_wasm_memory.initialized) {
        return;
    }
    
    printf("Cleaning up WASM memory manager...\n");
    
    // Check for leaks before cleanup
    int leaks = wasm_check_memory_leaks();
    
    // Free all tracked blocks
    WasmMemoryBlock* current = g_wasm_memory.blocks;
    while (current) {
        WasmMemoryBlock* next = current->next;
        free(current->ptr);  // Free the actual memory
        free(current);       // Free the tracking block
        current = next;
    }
    
    // Print final statistics
    printf("Final WASM memory statistics:\n");
    printf("  Peak usage: %.2f MB\n", g_wasm_memory.peak_usage / (1024.0 * 1024.0));
    printf("  Memory leaks: %d\n", leaks);
    
    // Reset state
    memset(&g_wasm_memory, 0, sizeof(g_wasm_memory));
    
    printf("WASM memory manager cleanup completed\n");
}