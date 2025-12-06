#include "core/memory.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

static void assert_double_close(double value, double expected, double tolerance) {
    double delta = fabs(value - expected);
    assert(delta <= tolerance);
}

static void test_allocation_reporting(void) {
    hyperionMemTrackInit();

    void *modelWeights = hyperionTrackedAlloc(2 * 1024 * 1024, "model_weights");
    void *activationCache = hyperionTrackedAlloc(3 * 1024 * 1024, "activation_cache");
    assert(modelWeights != NULL);
    assert(activationCache != NULL);

    HyperionMemoryStats stats = hyperionMemTrackSnapshot();
    assert(stats.totalAllocations == 2);
    assert(stats.outstandingAllocations == 2);
    assert(stats.peakBytes == 5 * 1024 * 1024);
    assert_double_close(stats.averageAllocationSize, 2.5 * 1024 * 1024, 1024);

    hyperionTrackedFree(modelWeights);
    hyperionTrackedFree(activationCache);
    hyperionMemTrackCleanup();
}

static void sleep_ms(unsigned long ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
#endif
}

static void test_lifetime_and_free(void) {
    hyperionMemTrackInit();

    void *buffer = hyperionTrackedAlloc(1024, "temp_buffer");
    assert(buffer != NULL);

    sleep_ms(2);

    hyperionTrackedFree(buffer);

    HyperionMemoryStats stats = hyperionMemTrackSnapshot();
    assert(stats.totalFrees == 1);
    assert(stats.outstandingAllocations == 0);
    assert(stats.currentBytes == 0);
    assert(stats.averageLifetimeMs >= 0.0);

    hyperionMemTrackCleanup();
}

static void test_report_output(void) {
    hyperionMemTrackInit();
    void *block = hyperionTrackedAlloc(4096, "report_block");
    assert(block != NULL);

    FILE *stream = tmpfile();
    assert(stream != NULL);

    hyperionMemTrackDumpReport(stream);
    fflush(stream);

    long length = 0;
    if (fseek(stream, 0, SEEK_END) == 0) {
        length = ftell(stream);
    }
    rewind(stream);

    char *buffer = NULL;
    if (length > 0) {
        buffer = (char *)malloc((size_t)length + 1);
        assert(buffer != NULL);
        size_t read = fread(buffer, 1, (size_t)length, stream);
        buffer[read] = '\0';
    }

    fclose(stream);

    assert(buffer != NULL);
    assert(strstr(buffer, "[memory]") != NULL);
    assert(strstr(buffer, "allocations") != NULL);

    free(buffer);
    hyperionTrackedFree(block);
    hyperionMemTrackCleanup();
}

static void test_bucket_accounting(void) {
    hyperionMemTrackInit();

    void *small = hyperionTrackedAlloc(32, "small_block");
    void *medium = hyperionTrackedAlloc(600, "medium_block");
    void *large = hyperionTrackedAlloc(90000, "large_block");

    assert(small != NULL && medium != NULL && large != NULL);

    HyperionMemoryStats stats = hyperionMemTrackSnapshot();
    assert(stats.peakBytes == 32 + 600 + 90000);
    assert(stats.bucketCounts[0] == 1);
    assert(stats.bucketCounts[2] == 1);
    assert(stats.bucketCounts[HYPERION_MEM_BUCKET_COUNT - 1] == 1);

    hyperionTrackedFree(medium);
    stats = hyperionMemTrackSnapshot();
    assert(stats.bucketCounts[2] == 0);
    assert(hyperionMemTrackGetPeakBytes() == 32 + 600 + 90000);

    size_t bucketSnapshot[HYPERION_MEM_BUCKET_COUNT] = {0};
    hyperionMemTrackGetBucketCounts(bucketSnapshot, HYPERION_MEM_BUCKET_COUNT);
    assert(bucketSnapshot[0] == 1);
    assert(bucketSnapshot[HYPERION_MEM_BUCKET_COUNT - 1] == 1);

    hyperionTrackedFree(small);
    hyperionTrackedFree(large);
    hyperionMemTrackCleanup();
}

void run_memory_tests(void) {
    test_allocation_reporting();
    test_lifetime_and_free();
    test_report_output();
    test_bucket_accounting();
    printf("All memory tracking tests passed.\n");
}
