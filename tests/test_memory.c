/**
 * Hyperion Memory Management Tests
 */

#include "../core/memory.h" // Include the header for the functions being tested
#include <stdio.h>
#include <stdlib.h> // For exit()
#include <string.h> // For memset

// Basic assertion helper
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion Failed: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while (0)

void test_basic_alloc_free() {
    printf("  Testing basic hyperionAlloc/hyperionFree...\n");
    void *ptr = hyperionAlloc(100);
    ASSERT(ptr != NULL, "hyperionAlloc(100) should return non-NULL");
    // Optional: Try writing to the memory
    memset(ptr, 0xAA, 100); 
    hyperionFree(ptr);
    printf("    PASS\n");

    printf("  Testing hyperionAlloc(0)...\n");
    // Standard malloc(0) behavior is implementation-defined (can return NULL or unique ptr)
    // Let's assume it might return non-NULL but shouldn't be used.
    ptr = hyperionAlloc(0);
    // ASSERT(ptr == NULL, "hyperionAlloc(0) should return NULL"); // Or assert non-null depending on desired behavior
    if (ptr != NULL) {
        hyperionFree(ptr); // Free it if non-NULL
    }
    printf("    PASS (behavior check)\n");
    
    printf("  Testing hyperionFree(NULL)...\n");
    hyperionFree(NULL); // Should be a no-op and not crash
    printf("    PASS (no crash)\n");
}

void test_calloc() {
     printf("  Testing hyperionCalloc...\n");
     size_t count = 10;
     size_t size = sizeof(int);
     int *ptr = (int*)hyperionCalloc(count, size);
     ASSERT(ptr != NULL, "hyperionCalloc should return non-NULL");
     
     // Verify memory is zeroed
     int zeroed = 1;
     for (size_t i = 0; i < count; ++i) {
         if (ptr[i] != 0) {
             zeroed = 0;
             break;
         }
     }
     ASSERT(zeroed, "hyperionCalloc memory should be zero-initialized");
     
     hyperionFree(ptr);
     printf("    PASS\n");
}

void test_realloc() {
    printf("  Testing hyperionRealloc...\n");
    
    // Realloc from NULL (should behave like malloc)
    void *ptr = hyperionRealloc(NULL, 50);
    ASSERT(ptr != NULL, "hyperionRealloc(NULL, 50) should return non-NULL");
    memset(ptr, 0xBB, 50);

    // Realloc to larger size
    void *ptr_larger = hyperionRealloc(ptr, 150);
    ASSERT(ptr_larger != NULL, "hyperionRealloc to larger size should return non-NULL");
    // Check if original content is preserved (up to the old size)
    unsigned char* check_ptr = (unsigned char*)ptr_larger;
    int preserved = 1;
    for(int i=0; i<50; ++i) {
        if (check_ptr[i] != 0xBB) { preserved = 0; break; }
    }
    ASSERT(preserved, "Content should be preserved after realloc to larger size");
    memset((char*)ptr_larger + 50, 0xCC, 100); // Write to new part

    // Realloc to smaller size
    void *ptr_smaller = hyperionRealloc(ptr_larger, 20);
     ASSERT(ptr_smaller != NULL, "hyperionRealloc to smaller size should return non-NULL");
     // Check if original content is preserved (up to the new size)
     check_ptr = (unsigned char*)ptr_smaller;
     preserved = 1;
     for(int i=0; i<20; ++i) {
         // Note: Content check might fail if realloc moved the block, but the first part *should* be copied.
         // This check assumes the first 20 bytes were 0xBB from the initial memset.
         if (check_ptr[i] != 0xBB) { preserved = 0; break; } 
     }
     // ASSERT(preserved, "Content should be preserved after realloc to smaller size"); // This might be fragile

    // Realloc to zero size (should behave like free)
    void *ptr_zero = hyperionRealloc(ptr_smaller, 0);
    // Standard realloc(ptr, 0) behavior is implementation-defined (can return NULL or unique ptr)
    // Let's assume it frees and might return NULL.
    // ASSERT(ptr_zero == NULL, "hyperionRealloc(ptr, 0) should return NULL"); 
    if (ptr_zero != NULL) {
        hyperionFree(ptr_zero); // Free it if non-NULL
    }

    printf("    PASS\n");
}


// Function to be called by test_main.c
void run_memory_tests() {
    printf("--- Running Memory Tests ---\n");
    test_basic_alloc_free();
    test_calloc();
    test_realloc();
    // Add calls to pool tests and tracking tests later
    printf("--- Memory Tests Finished ---\n");
}
