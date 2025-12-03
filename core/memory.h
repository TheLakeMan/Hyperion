#ifndef HYPERION_MEMORY_H
#define HYPERION_MEMORY_H

int hyperionMemTrackInit(void);
void hyperionMemTrackCleanup(void);
int hyperionMemTrackDumpLeaks(void);

#endif // HYPERION_MEMORY_H
