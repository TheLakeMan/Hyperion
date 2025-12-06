#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    void *ptr;
    size_t size;
    clock_t start;
    char label[32];
} AllocationRecord;

static AllocationRecord *records = NULL;
static size_t recordCount = 0;
static size_t recordCapacity = 0;

static size_t totalAllocations = 0;
static size_t totalFrees = 0;
static size_t totalBytesAllocated = 0;
static size_t totalBytesFreed = 0;
static size_t currentBytes = 0;
static size_t peakBytes = 0;
static double totalLifetimeMs = 0.0;

static int ensure_capacity(void) {
    if (recordCount < recordCapacity) {
        return 0;
    }

    size_t newCapacity = recordCapacity == 0 ? 16 : recordCapacity * 2;
    AllocationRecord *newRecords = realloc(records, newCapacity * sizeof(AllocationRecord));
    if (!newRecords) {
        return -1;
    }

    records = newRecords;
    recordCapacity = newCapacity;
    return 0;
}

int hyperionMemTrackInit(void) {
    free(records);
    records = NULL;
    recordCount = 0;
    recordCapacity = 0;
    totalAllocations = 0;
    totalFrees = 0;
    totalBytesAllocated = 0;
    totalBytesFreed = 0;
    currentBytes = 0;
    peakBytes = 0;
    totalLifetimeMs = 0.0;
    return 0;
}

void hyperionMemTrackCleanup(void) {
    free(records);
    records = NULL;
    recordCount = 0;
    recordCapacity = 0;
    totalAllocations = 0;
    totalFrees = 0;
    totalBytesAllocated = 0;
    totalBytesFreed = 0;
    currentBytes = 0;
    peakBytes = 0;
    totalLifetimeMs = 0.0;
}

void *hyperionTrackedAlloc(size_t size, const char *label) {
    void *ptr = malloc(size);
    if (!ptr) {
        return NULL;
    }

    if (ensure_capacity() != 0) {
        free(ptr);
        return NULL;
    }

    AllocationRecord *record = &records[recordCount++];
    record->ptr = ptr;
    record->size = size;
    record->start = clock();
    if (label) {
        strncpy(record->label, label, sizeof(record->label) - 1);
        record->label[sizeof(record->label) - 1] = '\0';
    } else {
        record->label[0] = '\0';
    }

    totalAllocations++;
    totalBytesAllocated += size;
    currentBytes += size;
    if (currentBytes > peakBytes) {
        peakBytes = currentBytes;
    }

    return ptr;
}

void hyperionTrackedFree(void *ptr) {
    if (!ptr || recordCount == 0) {
        free(ptr);
        return;
    }

    size_t index = recordCount;
    for (size_t i = 0; i < recordCount; ++i) {
        if (records[i].ptr == ptr) {
            index = i;
            break;
        }
    }

    if (index == recordCount) {
        free(ptr);
        return;
    }

    AllocationRecord record = records[index];
    records[index] = records[recordCount - 1];
    recordCount--;

    free(ptr);

    totalFrees++;
    totalBytesFreed += record.size;
    if (currentBytes >= record.size) {
        currentBytes -= record.size;
    } else {
        currentBytes = 0;
    }

    clock_t elapsed = clock() - record.start;
    totalLifetimeMs += ((double)elapsed / CLOCKS_PER_SEC) * 1000.0;
}

static double average_allocation_size(void) {
    if (totalAllocations == 0) {
        return 0.0;
    }
    return (double)totalBytesAllocated / (double)totalAllocations;
}

static double average_lifetime_ms(void) {
    if (totalFrees == 0) {
        return 0.0;
    }
    return totalLifetimeMs / (double)totalFrees;
}

static void format_leak_record(FILE *out, const AllocationRecord *record) {
    if (!out || !record) {
        return;
    }

    char label[sizeof(record->label)];
    strncpy(label, record->label, sizeof(label) - 1);
    label[sizeof(label) - 1] = '\0';

    const char *labelToPrint = label[0] != '\0' ? label : "(no label)";
    double timestampMs = ((double)record->start / CLOCKS_PER_SEC) * 1000.0;

    fprintf(out, "[memory] leak: ptr=%p size=%zu label=\"%s\" allocated_at=%.3f ms\n",
            record->ptr, record->size, labelToPrint, timestampMs);
}

HyperionMemoryStats hyperionMemTrackSnapshot(void) {
    HyperionMemoryStats stats;
    stats.totalAllocations = totalAllocations;
    stats.totalFrees = totalFrees;
    stats.totalBytesAllocated = totalBytesAllocated;
    stats.totalBytesFreed = totalBytesFreed;
    stats.currentBytes = currentBytes;
    stats.peakBytes = peakBytes;
    stats.averageAllocationSize = average_allocation_size();
    stats.averageLifetimeMs = average_lifetime_ms();
    stats.outstandingAllocations = recordCount;
    return stats;
}

void hyperionMemTrackDumpReport(FILE *out) {
    if (!out) {
        return;
    }

    HyperionMemoryStats stats = hyperionMemTrackSnapshot();
    fprintf(out, "[memory] allocations: %zu frees: %zu outstanding: %zu\n",
            stats.totalAllocations, stats.totalFrees, stats.outstandingAllocations);
    fprintf(out, "[memory] bytes allocated: %zu freed: %zu current: %zu peak: %zu\n",
            stats.totalBytesAllocated, stats.totalBytesFreed, stats.currentBytes, stats.peakBytes);
    fprintf(out, "[memory] average allocation size: %.2f bytes\n", stats.averageAllocationSize);
    fprintf(out, "[memory] average lifetime: %.2f ms\n", stats.averageLifetimeMs);
}

int hyperionMemTrackDumpLeaks(FILE *out) {
    if (!out) {
        out = stderr;
    }

    for (size_t i = 0; i < recordCount; ++i) {
        format_leak_record(out, &records[i]);
    }

    return (int)recordCount;
}
