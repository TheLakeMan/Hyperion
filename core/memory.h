/**
 * Hyperion Memory Management Header
 * 
 * This header defines memory management functions for Hyperion, including
 * memory tracking and optimized allocation for small embedded systems.
 */

#ifndef HYPERION_MEMORY_H
#define HYPERION_MEMORY_H

#include <stddef.h>

/* ----------------- Memory Allocation ----------------- */

/**
 * Allocate memory
 * 
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* hyperionAlloc(size_t size);

/**
 * Reallocate memory
 * 
 * @param ptr Pointer to memory to reallocate
 * @param size New size in bytes
 * @return Pointer to reallocated memory or NULL on failure
 */
void* hyperionRealloc(void *ptr, size_t size);

/**
 * Free memory
 * 
 * @param ptr Pointer to memory to free
 */
void hyperionFree(void *ptr);

/**
 * Allocate zero-initialized memory
 * 
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* hyperionCalloc(size_t count, size_t size);

/* ----------------- Memory Pool ----------------- */

/**
 * Initialize memory pool
 * 
 * @param size Size of memory pool in bytes
 * @return 0 on success, non-zero on error
 */
int hyperionMemPoolInit(size_t size);

/**
 * Clean up memory pool
 */
void hyperionMemPoolCleanup();

/**
 * Allocate memory from pool
 * 
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* hyperionMemPoolAlloc(size_t size);

/**
 * Free memory from pool
 * 
 * @param ptr Pointer to memory to free
 */
void hyperionMemPoolFree(void *ptr);

/**
 * Reset memory pool (free all allocations)
 */
void hyperionMemPoolReset();

/**
 * Get memory pool statistics
 * 
 * @param totalSize Pointer to store total pool size
 * @param usedSize Pointer to store used pool size
 * @param peakSize Pointer to store peak used pool size
 * @param allocCount Pointer to store allocation count
 */
void hyperionMemPoolStats(size_t *totalSize, size_t *usedSize, 
                       size_t *peakSize, size_t *allocCount);

/* ----------------- Memory Tracking ----------------- */

/**
 * Initialize memory tracking
 * 
 * @return 0 on success, non-zero on error
 */
int hyperionMemTrackInit();

/**
 * Clean up memory tracking
 */
void hyperionMemTrackCleanup();

/**
 * Track memory allocation
 * 
 * @param ptr Pointer to allocated memory
 * @param size Size of allocation
 * @param file Source file name
 * @param line Source line number
 */
void hyperionMemTrackAlloc(void *ptr, size_t size, const char *file, int line);

/**
 * Track memory free
 * 
 * @param ptr Pointer to freed memory
 */
void hyperionMemTrackFree(void *ptr);

/**
 * Dump memory leaks
 * 
 * @return Number of leaks found
 */
int hyperionMemTrackDumpLeaks();

/**
 * Get memory tracking statistics
 * 
 * @param allocCount Pointer to store allocation count
 * @param allocSize Pointer to store total allocation size
 * @param freeCount Pointer to store free count
 * @param freeSize Pointer to store total free size
 */
void hyperionMemTrackStats(size_t *allocCount, size_t *allocSize, 
                        size_t *freeCount, size_t *freeSize);

/* ----------------- Macros ----------------- */

/* Memory tracking macros */
#ifdef HYPERION_MEMORY_TRACKING
void* hyperionTrackedMalloc(size_t size, const char *file, int line);
void* hyperionTrackedRealloc(void *ptr, size_t size, const char *file, int line);
void* hyperionTrackedCalloc(size_t count, size_t size, const char *file, int line);
void  hyperionTrackedFree(void *ptr);

#define HYPERION_MALLOC(size) hyperionTrackedMalloc((size), __FILE__, __LINE__)
#define HYPERION_REALLOC(ptr, size) hyperionTrackedRealloc((ptr), (size), __FILE__, __LINE__)
#define HYPERION_FREE(ptr) hyperionTrackedFree((ptr))
#define HYPERION_CALLOC(count, size) \
    hyperionTrackedCalloc((count), (size), __FILE__, __LINE__)
#else
#define HYPERION_MALLOC(size) hyperionAlloc((size))
#define HYPERION_REALLOC(ptr, size) hyperionRealloc((ptr), (size))
#define HYPERION_FREE(ptr) hyperionFree((ptr))
#define HYPERION_CALLOC(count, size) hyperionCalloc((count), (size))
#endif

#endif /* HYPERION_MEMORY_H */