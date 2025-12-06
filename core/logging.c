#include "logging.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HyperionLogLevel currentLevel = HYPERION_LOG_INFO;
static bool jsonLoggingEnabled = false;
static FILE *defaultStream = NULL;

static const char *level_to_string(HyperionLogLevel level) {
    switch (level) {
    case HYPERION_LOG_DEBUG:
        return "DEBUG";
    case HYPERION_LOG_INFO:
        return "INFO";
    case HYPERION_LOG_WARN:
        return "WARN";
    case HYPERION_LOG_ERROR:
        return "ERROR";
    case HYPERION_LOG_NONE:
    default:
        return "NONE";
    }
}

static const char *level_to_lower(HyperionLogLevel level) {
    switch (level) {
    case HYPERION_LOG_DEBUG:
        return "debug";
    case HYPERION_LOG_INFO:
        return "info";
    case HYPERION_LOG_WARN:
        return "warn";
    case HYPERION_LOG_ERROR:
        return "error";
    case HYPERION_LOG_NONE:
    default:
        return "none";
    }
}

static char *escape_json_message(const char *message) {
    size_t length = strlen(message);
    size_t extra = 0;

    for (size_t i = 0; i < length; ++i) {
        char c = message[i];
        switch (c) {
        case '"':
        case '\\':
        case '\n':
        case '\r':
        case '\t':
            extra++;
            break;
        default:
            break;
        }
    }

    char *escaped = (char *)malloc(length + extra + 1);
    if (!escaped) {
        return NULL;
    }

    char *dest = escaped;
    for (size_t i = 0; i < length; ++i) {
        char c = message[i];
        switch (c) {
        case '"':
            *dest++ = '\\';
            *dest++ = '"';
            break;
        case '\\':
            *dest++ = '\\';
            *dest++ = '\\';
            break;
        case '\n':
            *dest++ = '\\';
            *dest++ = 'n';
            break;
        case '\r':
            *dest++ = '\\';
            *dest++ = 'r';
            break;
        case '\t':
            *dest++ = '\\';
            *dest++ = 't';
            break;
        default:
            *dest++ = c;
            break;
        }
    }

    *dest = '\0';
    return escaped;
}

static void log_message(FILE *stream, HyperionLogLevel level, const char *message) {
    FILE *out = stream ? stream : stderr;

    if (jsonLoggingEnabled) {
        char *escaped = escape_json_message(message);
        if (!escaped) {
            return;
        }
        fprintf(out, "{\"level\":\"%s\",\"message\":\"%s\"}\n", level_to_lower(level), escaped);
        free(escaped);
    } else {
        fprintf(out, "[%s] %s\n", level_to_string(level), message);
    }

    fflush(out);
}

static void vlog_message(FILE *stream, HyperionLogLevel level, const char *format, va_list args) {
    if (level < currentLevel || level == HYPERION_LOG_NONE) {
        return;
    }

    char stackBuffer[256];
    va_list argsCopy;
    va_copy(argsCopy, args);
    int needed = vsnprintf(stackBuffer, sizeof(stackBuffer), format, argsCopy);
    va_end(argsCopy);

    if (needed < 0) {
        return;
    }

    char *messageBuffer = NULL;
    if ((size_t)needed >= sizeof(stackBuffer)) {
        messageBuffer = (char *)malloc((size_t)needed + 1);
        if (!messageBuffer) {
            return;
        }
        vsnprintf(messageBuffer, (size_t)needed + 1, format, args);
    }

    const char *finalMessage = messageBuffer ? messageBuffer : stackBuffer;
    log_message(stream ? stream : defaultStream, level, finalMessage);
    free(messageBuffer);
}

void hyperionLogSetLevel(HyperionLogLevel level) {
    currentLevel = level;
}

HyperionLogLevel hyperionLogGetLevel(void) {
    return currentLevel;
}

void hyperionLogEnableJson(int enable) {
    jsonLoggingEnabled = enable != 0;
}

int hyperionLogIsJsonEnabled(void) {
    return jsonLoggingEnabled ? 1 : 0;
}

void hyperionLogSetStream(FILE *stream) {
    defaultStream = stream;
}

FILE *hyperionLogGetStream(void) {
    return defaultStream;
}

void hyperionLogf(HyperionLogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vlog_message(defaultStream, level, format, args);
    va_end(args);
}

void hyperionLogfTo(FILE *stream, HyperionLogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vlog_message(stream, level, format, args);
    va_end(args);
}
