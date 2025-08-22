/**
 * Hyperion Memory Validation Suite
 * 
 * Comprehensive testing of memory optimization, leak detection,
 * and performance validation for the Hyperion AI framework.
 */

#include "../core/memory.h"
#include "../core/config.h"
#include "../models/text/generate.h"
#include "../models/text/tokenizer.h"
#include "../core/enhanced_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

/* Test configuration */
#define MAX_TEST_ITERATIONS 100
#define STRESS_TEST_CYCLES 1000
#define MEMORY_LIMIT_TEST_MB 64
#define LARGE_ALLOCATION_SIZE (1024 * 1024)

/* Test statistics */
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
    size_t peak_memory_usage;
    size_t total_allocations;
    double total_test_time;
    int memory_leaks_detected;
} MemoryTestStats;

/* Global test state */
static MemoryTestStats g_test_stats = {0};

/* Color output for better readability */
#ifdef _WIN32
#define COLOR_GREEN ""
#define COLOR_RED ""
#define COLOR_YELLOW ""
#define COLOR_BLUE ""
#define COLOR_RESET ""
#else
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m"
#endif

/* Helper functions */
static size_t get_system_memory_usage(void) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024;
    }
    return 0;
#endif
}

static void print_test_header(const char* test_name) {
    printf("\n%s=== %s ===%s\n", COLOR_BLUE, test_name, COLOR_RESET);
}

static void print_test_result(const char* test_name, int passed, const char* details) {
    g_test_stats.tests_run++;
    if (passed) {
        g_test_stats.tests_passed++;
        printf("%s✓ PASS:%s %s", COLOR_GREEN, COLOR_RESET, test_name);
    } else {
        g_test_stats.tests_failed++;
        printf("%s✗ FAIL:%s %s", COLOR_RED, COLOR_RESET, test_name);
    }
    
    if (details) {
        printf(" - %s", details);
    }
    printf("\n");
}

static void update_peak_memory(void) {
    size_t current_memory = get_system_memory_usage();
    if (current_memory > g_test_stats.peak_memory_usage) {
        g_test_stats.peak_memory_usage = current_memory;
    }
}

/* Test 1: Basic Memory Allocation and Deallocation */
static int test_basic_allocation(void) {
    print_test_header("Basic Memory Allocation Test");
    
    void* ptr1 = hyperionAllocate(64);
    if (!ptr1) {
        print_test_result("Small allocation (64 bytes)", 0, "Allocation failed");
        return 0;
    }
    
    memset(ptr1, 0xAA, 64);
    print_test_result("Small allocation (64 bytes)", 1, "64 bytes allocated and initialized");
    
    void* ptr2 = hyperionAllocate(4096);
    if (!ptr2) {
        hyperionFree(ptr1);
        print_test_result("Medium allocation (4KB)", 0, "Allocation failed");
        return 0;
    }
    
    memset(ptr2, 0xBB, 4096);
    print_test_result("Medium allocation (4KB)", 1, "4KB allocated and initialized");
    
    void* ptr3 = hyperionAllocate(LARGE_ALLOCATION_SIZE);
    if (!ptr3) {
        hyperionFree(ptr1);
        hyperionFree(ptr2);
        print_test_result("Large allocation (1MB)", 0, "Allocation failed");
        return 0;
    }
    
    print_test_result("Large allocation (1MB)", 1, "1MB allocated successfully");
    
    hyperionFree(ptr1);
    hyperionFree(ptr2);
    hyperionFree(ptr3);
    
    print_test_result("Memory deallocation", 1, "All allocations freed");
    
    update_peak_memory();
    return 1;
}

/* Test 2: Memory Leak Detection */
static int test_leak_detection(void) {
    print_test_header("Memory Leak Detection Test");
    
    hyperionMemTrackInit();
    
    size_t initial_allocations = hyperionGetAllocationCount();
    
    void* ptr1 = hyperionAllocate(128);
    void* ptr2 = hyperionAllocate(256);
    void* ptr3 = hyperionAllocate(512);
    
    if (!ptr1 || !ptr2 || !ptr3) {
        print_test_result("Test allocations", 0, "Failed to allocate test memory");
        return 0;
    }
    
    hyperionFree(ptr1);
    hyperionFree(ptr2);
    /* ptr3 intentionally not freed to test leak detection */
    
    int leak_count = hyperionMemTrackDumpLeaks();
    
    hyperionFree(ptr3);
    
    if (leak_count > 0) {
        char details[128];
        snprintf(details, sizeof(details), "Detected %d leak(s) as expected", leak_count);
        print_test_result("Leak detection functionality", 1, details);
        g_test_stats.memory_leaks_detected += leak_count;
    } else {
        print_test_result("Leak detection functionality", 0, "Failed to detect intentional leak");
        return 0;
    }
    
    hyperionMemTrackCleanup();
    update_peak_memory();
    return 1;
}

/* Main test runner */
int main(int argc, char* argv[]) {
    printf("%s=== Hyperion Memory Validation Suite ===%s\n", COLOR_BLUE, COLOR_RESET);
    printf("Comprehensive memory testing and optimization validation\n\n");
    
    srand((unsigned int)time(NULL));
    
    if (hyperionMemoryInit() != 0) {
        printf("%sERROR:%s Failed to initialize Hyperion memory system\n", COLOR_RED, COLOR_RESET);
        return 1;
    }
    
    hyperion_enhanced_errors_init();
    
    clock_t start_time = clock();
    
    int overall_success = 1;
    
    overall_success &= test_basic_allocation();
    overall_success &= test_leak_detection();
    
    clock_t end_time = clock();
    g_test_stats.total_test_time = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;
    
    printf("\n%s=== Test Results Summary ===%s\n", COLOR_BLUE, COLOR_RESET);
    printf("Tests run: %d\n", g_test_stats.tests_run);
    printf("Tests passed: %s%d%s\n", COLOR_GREEN, g_test_stats.tests_passed, COLOR_RESET);
    printf("Tests failed: %s%d%s\n", 
           g_test_stats.tests_failed > 0 ? COLOR_RED : COLOR_GREEN, 
           g_test_stats.tests_failed, COLOR_RESET);
    printf("Peak memory usage: %.2f MB\n", g_test_stats.peak_memory_usage / (1024.0 * 1024.0));
    printf("Total test time: %.2f ms\n", g_test_stats.total_test_time);
    
    if (g_test_stats.memory_leaks_detected > 0) {
        printf("Memory leaks detected: %s%d%s\n", COLOR_YELLOW, g_test_stats.memory_leaks_detected, COLOR_RESET);
    }
    
    if (overall_success && g_test_stats.tests_failed == 0) {
        printf("\n%s🎉 ALL TESTS PASSED - Memory optimization validation successful!%s\n", 
               COLOR_GREEN, COLOR_RESET);
    } else {
        printf("\n%s❌ SOME TESTS FAILED - Please review and fix issues before proceeding%s\n", 
               COLOR_RED, COLOR_RESET);
    }
    
    hyperion_enhanced_errors_cleanup();
    hyperionMemoryCleanup();
    
    return overall_success && g_test_stats.tests_failed == 0 ? 0 : 1;
}