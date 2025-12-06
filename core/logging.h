#ifndef HYPERION_LOGGING_H
#define HYPERION_LOGGING_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HYPERION_LOG_LEVEL_ERROR = 0,
    HYPERION_LOG_LEVEL_WARN,
    HYPERION_LOG_LEVEL_INFO,
    HYPERION_LOG_LEVEL_DEBUG,
    HYPERION_LOG_LEVEL_TRACE
} HyperionLogLevel;

void hyperionLog(HyperionLogLevel level, const char *format, ...);

#define hyperionLogError(...) hyperionLog(HYPERION_LOG_LEVEL_ERROR, __VA_ARGS__)
#define hyperionLogWarn(...)  hyperionLog(HYPERION_LOG_LEVEL_WARN, __VA_ARGS__)
#define hyperionLogInfo(...)  hyperionLog(HYPERION_LOG_LEVEL_INFO, __VA_ARGS__)
#define hyperionLogDebug(...) hyperionLog(HYPERION_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define hyperionLogTrace(...) hyperionLog(HYPERION_LOG_LEVEL_TRACE, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // HYPERION_LOGGING_H
