#ifndef HYPERION_LOGGING_H
#define HYPERION_LOGGING_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HYPERION_LOG_DEBUG = 0,
    HYPERION_LOG_INFO = 1,
    HYPERION_LOG_WARN = 2,
    HYPERION_LOG_ERROR = 3,
    HYPERION_LOG_NONE = 4
} HyperionLogLevel;

void hyperionLogSetLevel(HyperionLogLevel level);
HyperionLogLevel hyperionLogGetLevel(void);

void hyperionLogEnableJson(int enable);
int hyperionLogIsJsonEnabled(void);

void hyperionLogSetStream(FILE *stream);
FILE *hyperionLogGetStream(void);

void hyperionLogf(HyperionLogLevel level, const char *format, ...);
void hyperionLogfTo(FILE *stream, HyperionLogLevel level, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // HYPERION_LOGGING_H
