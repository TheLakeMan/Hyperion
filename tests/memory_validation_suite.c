#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../core/memory.h"
#include "../utils/memory_pool.h"

// Test basic memory allocation/deallocation
int test_basic_allocation() {
    printf("Running basic allocation test...\n");
    
    void* ptr1 = hyperionAlloc(1024);
    assert(ptr1 != NULL);
    
    void* ptr2 = hyperionAlloc(2048);
    assert(ptr2 != NULL);
    
    hyperionFree(ptr1);
    hyperionFree(ptr2);
    
    printf("✓ Basic allocation test passed\n");
    return 0;
}

// Test memory pool functionality
int test_memory_pool() {
    printf("Running memory pool test...\n");
    
    int result = hyperionMemPoolInit(64 * 1024); // 64KB pool
    assert(result == 0);
    
    void* ptr1 = hyperionMemPoolAlloc(1024);
    assert(ptr1 != NULL);
    
    void* ptr2 = hyperionMemPoolAlloc(2048);
    assert(ptr2 != NULL);
    
    hyperionMemPoolFree(ptr1);
    hyperionMemPoolFree(ptr2);
    
    hyperionMemPoolCleanup();
    
    printf("✓ Memory pool test passed\n");
    return 0;
}

// Test memory leak detection
int test_leak_detection() {
    printf("Running leak detection test...\n");
    
    hyperionMemTrackInit();
    
    // Intentionally leak memory for testing
    void* leaked_ptr = hyperionAlloc(512);
    (void)leaked_ptr; // Suppress unused variable warning
    
    int leak_count = hyperionMemTrackDumpLeaks();
    assert(leak_count > 0);
    
    printf("✓ Leak detection test passed (detected %d leaks)\n", leak_count);
    
    // Clean up for next tests
    hyperionMemTrackCleanup();
    return 0;
}

// Test stress allocation
int test_stress_allocation() {
    printf("Running stress allocation test...\n");
    
    const int num_allocs = 1000;
    void** ptrs = malloc(num_allocs * sizeof(void*));
    
    // Allocate many small blocks
    for (int i = 0; i < num_allocs; i++) {
        ptrs[i] = hyperionAlloc(64 + (i % 512));
        assert(ptrs[i] != NULL);
    }
    
    // Free all blocks
    for (int i = 0; i < num_allocs; i++) {
        hyperionFree(ptrs[i]);
    }
    
    free(ptrs);
    
    printf("✓ Stress allocation test passed (%d allocations)\n", num_allocs);
    return 0;
}

// Test memory usage monitoring
int test_memory_monitoring() {
    printf("Running memory monitoring test...\n");
    
    size_t alloc_count, alloc_size, free_count, free_size;
    hyperionMemTrackInit();
    
    hyperionMemTrackStats(&alloc_count, &alloc_size, &free_count, &free_size);
    size_t initial_allocs = alloc_count;
    
    void* large_block = hyperionAlloc(32 * 1024); // 32KB
    assert(large_block != NULL);
    
    hyperionMemTrackStats(&alloc_count, &alloc_size, &free_count, &free_size);
    assert(alloc_count > initial_allocs);
    
    hyperionFree(large_block);
    
    hyperionMemTrackStats(&alloc_count, &alloc_size, &free_count, &free_size);
    assert(free_count > 0);
    
    hyperionMemTrackCleanup();
    
    printf("✓ Memory monitoring test passed\n");
    return 0;
}

int main() {
    printf("========================================\n");
    printf("Hyperion Memory Validation Suite\n");
    printf("========================================\n");
    
    // Initialize memory tracking
    hyperionMemTrackInit();
    
    int result = 0;
    
    result += test_basic_allocation();
    result += test_memory_pool();
    result += test_leak_detection();
    result += test_stress_allocation();
    result += test_memory_monitoring();
    
    // Cleanup
    hyperionMemTrackCleanup();
    
    if (result == 0) {
        printf("\n✓ All memory validation tests passed!\n");
    } else {
        printf("\n✗ %d memory validation tests failed!\n", result);
    }
    
    printf("========================================\n");
    
    return result;
}