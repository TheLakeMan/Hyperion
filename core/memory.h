#ifndef HYPERION_MEMORY_H
#define HYPERION_MEMORY_H

#include <stddef.h>
#include <stdio.h>

#define HYPERION_MEM_BUCKET_COUNT 7
extern const size_t hyperionMemBucketThresholds[HYPERION_MEM_BUCKET_COUNT];

typedef struct {
    size_t totalAllocations;
    size_t totalFrees;
    size_t totalBytesAllocated;
    size_t totalBytesFreed;
    size_t currentBytes;
    size_t peakBytes;
    double averageAllocationSize;
    double averageLifetimeMs;
    size_t outstandingAllocations;
    size_t bucketCounts[HYPERION_MEM_BUCKET_COUNT];
} HyperionMemoryStats;

int hyperionMemTrackInit(void);
void hyperionMemTrackCleanup(void);

void *hyperionTrackedAlloc(size_t size, const char *label);
void hyperionTrackedFree(void *ptr);

HyperionMemoryStats hyperionMemTrackSnapshot(void);
void hyperionMemTrackDumpReport(FILE *out);
void hyperionMemTrackDumpBucketReport(FILE *out);
int hyperionMemTrackDumpLeaks(void);
size_t hyperionMemTrackGetPeakBytes(void);
void hyperionMemTrackGetBucketCounts(size_t *outCounts, size_t count);

#endif // HYPERION_MEMORY_H
