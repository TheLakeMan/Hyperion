#include "memory.h"

static int outstanding_allocations = 0;

int hyperionMemTrackInit(void) {
    // Stub that would initialize allocation tracking
    outstanding_allocations = 0;
    return 0;
}

void hyperionMemTrackCleanup(void) {
    // Stub cleanup
}

int hyperionMemTrackDumpLeaks(void) {
    // Stub leak report
    return outstanding_allocations;
}
