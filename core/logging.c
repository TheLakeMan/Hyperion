#include "logging.h"
#include <stdio.h>
#include <time.h>

static const char *hyperionLogLevelLabel(HyperionLogLevel level) {
    switch (level) {
        case HYPERION_LOG_LEVEL_ERROR:
            return "ERROR";
        case HYPERION_LOG_LEVEL_WARN:
            return "WARN";
        case HYPERION_LOG_LEVEL_INFO:
            return "INFO";
        case HYPERION_LOG_LEVEL_DEBUG:
            return "DEBUG";
        case HYPERION_LOG_LEVEL_TRACE:
            return "TRACE";
        default:
            return "UNKNOWN";
    }
}

void hyperionLog(HyperionLogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timeBuffer[32];
    if (tm_info != NULL) {
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        timeBuffer[0] = '\0';
    }

    fprintf(stderr, "%s [hyperion] %s: ", timeBuffer, hyperionLogLevelLabel(level));
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
}
