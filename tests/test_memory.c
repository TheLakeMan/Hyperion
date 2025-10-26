#include "test_framework.h"
#include "../core/memory.h"

#include <string.h>

HYPERION_TEST(test_memory_basic_alloc)
{
    void *ptr = hyperionAlloc(100);
    HYPERION_ASSERT(ptr != NULL, "hyperionAlloc should return memory");
    memset(ptr, 0xAA, 100);
    hyperionFree(ptr);

    ptr = hyperionAlloc(0);
    if (ptr) {
        hyperionFree(ptr);
    }

    hyperionFree(NULL);
    return 0;
}

HYPERION_TEST(test_memory_calloc_zero_init)
{
    const size_t count = 16;
    int          *ptr   = (int *)hyperionCalloc(count, sizeof(int));
    HYPERION_ASSERT(ptr != NULL, "hyperionCalloc should succeed");

    for (size_t i = 0; i < count; ++i) {
        HYPERION_ASSERT(ptr[i] == 0, "hyperionCalloc must zero-initialize");
    }

    hyperionFree(ptr);
    return 0;
}

HYPERION_TEST(test_memory_realloc_preserves_content)
{
    unsigned char *ptr = (unsigned char *)hyperionRealloc(NULL, 32);
    HYPERION_ASSERT(ptr != NULL, "hyperionRealloc(NULL, size) should allocate");
    memset(ptr, 0x5A, 32);

    unsigned char *larger = (unsigned char *)hyperionRealloc(ptr, 64);
    HYPERION_ASSERT(larger != NULL, "hyperionRealloc to larger size should succeed");
    for (int i = 0; i < 32; ++i) {
        HYPERION_ASSERT(larger[i] == 0x5A, "Existing data must be preserved");
    }

    unsigned char *smaller = (unsigned char *)hyperionRealloc(larger, 16);
    HYPERION_ASSERT(smaller != NULL, "hyperionRealloc to smaller size should succeed");

    unsigned char *freed = (unsigned char *)hyperionRealloc(smaller, 0);
    if (freed) {
        hyperionFree(freed);
    }

    return 0;
}

HYPERION_TEST(test_memory_pool_lifecycle)
{
    hyperionMemPoolCleanup();

    HYPERION_ASSERT(hyperionMemPoolInit(256) == 0, "hyperionMemPoolInit should succeed");

    size_t total = 0, used = 0, peak = 0, count = 0;
    hyperionMemPoolStats(&total, &used, &peak, &count);
    HYPERION_ASSERT(total == 256, "Pool total size mismatch after init");
    HYPERION_ASSERT(used == 0, "Pool used size should start at zero");

    void *blockA = hyperionMemPoolAlloc(64);
    HYPERION_ASSERT(blockA != NULL, "Pool allocation should succeed");

    hyperionMemPoolStats(&total, &used, &peak, &count);
    HYPERION_ASSERT(used >= 64, "Used size should reflect allocation");
    HYPERION_ASSERT(count == 1, "Allocation count should increment");

    hyperionMemPoolReset();
    hyperionMemPoolStats(&total, &used, &peak, &count);
    HYPERION_ASSERT(used == 0, "Used size should reset to zero");
    HYPERION_ASSERT(count == 0, "Allocation count should reset to zero");

    hyperionMemPoolCleanup();
    hyperionMemPoolStats(&total, &used, &peak, &count);
    HYPERION_ASSERT(total == 0, "Pool total should be zero after cleanup");

    return 0;
}

HYPERION_TEST(test_memory_pool_out_of_memory)
{
    hyperionMemPoolCleanup();
    HYPERION_ASSERT(hyperionMemPoolInit(64) == 0, "Pool init should succeed");

    void *blockA = hyperionMemPoolAlloc(48);
    HYPERION_ASSERT(blockA != NULL, "First allocation should succeed");

    void *blockB = hyperionMemPoolAlloc(32);
    HYPERION_ASSERT(blockB == NULL, "Second allocation should fail when pool exhausted");

    hyperionMemPoolCleanup();
    return 0;
}

const HyperionTestCase g_memory_tests[] = {
    {"memory_basic_alloc", "core", test_memory_basic_alloc},
    {"memory_calloc_zero_init", "core", test_memory_calloc_zero_init},
    {"memory_realloc_preserves_content", "core", test_memory_realloc_preserves_content},
    {"memory_pool_lifecycle", "core", test_memory_pool_lifecycle},
    {"memory_pool_out_of_memory", "core", test_memory_pool_out_of_memory},
};

const size_t g_memory_test_count = sizeof(g_memory_tests) / sizeof(g_memory_tests[0]);
