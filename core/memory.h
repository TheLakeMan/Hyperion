#ifndef HYPERION_MEMORY_H
#define HYPERION_MEMORY_H

#include <stddef.h>
#include <stdio.h>

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
} HyperionMemoryStats;

int hyperionMemTrackInit(void);
void hyperionMemTrackCleanup(void);

void *hyperionTrackedAlloc(size_t size, const char *label);
void hyperionTrackedFree(void *ptr);

HyperionMemoryStats hyperionMemTrackSnapshot(void);
void hyperionMemTrackDumpReport(FILE *out);
int hyperionMemTrackDumpLeaks(void);

#endif // HYPERION_MEMORY_H
