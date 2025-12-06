#include "core/logging.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_stream(FILE *stream) {
    fflush(stream);
    long length = 0;
    if (fseek(stream, 0, SEEK_END) == 0) {
        length = ftell(stream);
    }
    rewind(stream);

    if (length <= 0) {
        return NULL;
    }

    char *buffer = (char *)malloc((size_t)length + 1);
    assert(buffer != NULL);
    size_t read = fread(buffer, 1, (size_t)length, stream);
    buffer[read] = '\0';
    return buffer;
}

static void test_severity_filtering(void) {
    HyperionLogLevel originalLevel = hyperionLogGetLevel();
    FILE *originalStream = hyperionLogGetStream();
    int originalJson = hyperionLogIsJsonEnabled();

    FILE *stream = tmpfile();
    assert(stream != NULL);

    hyperionLogSetLevel(HYPERION_LOG_WARN);
    hyperionLogSetStream(stream);
    hyperionLogEnableJson(0);

    hyperionLogf(HYPERION_LOG_INFO, "info message should be filtered");
    hyperionLogf(HYPERION_LOG_ERROR, "error message should appear");

    char *output = read_stream(stream);
    assert(output != NULL);
    assert(strstr(output, "error message should appear") != NULL);
    assert(strstr(output, "info message") == NULL);

    free(output);
    fclose(stream);

    hyperionLogSetLevel(originalLevel);
    hyperionLogSetStream(originalStream);
    hyperionLogEnableJson(originalJson);
}

static void test_json_formatting(void) {
    HyperionLogLevel originalLevel = hyperionLogGetLevel();
    FILE *originalStream = hyperionLogGetStream();
    int originalJson = hyperionLogIsJsonEnabled();

    FILE *stream = tmpfile();
    assert(stream != NULL);

    hyperionLogSetLevel(HYPERION_LOG_DEBUG);
    hyperionLogSetStream(stream);
    hyperionLogEnableJson(1);

    hyperionLogf(HYPERION_LOG_INFO, "json format test");

    char *output = read_stream(stream);
    assert(output != NULL);
    assert(strstr(output, "\"level\":\"info\"") != NULL);
    assert(strstr(output, "json format test") != NULL);
    assert(output[0] == '{');

    free(output);
    fclose(stream);

    hyperionLogSetLevel(originalLevel);
    hyperionLogSetStream(originalStream);
    hyperionLogEnableJson(originalJson);
}

void run_logging_tests(void) {
    test_severity_filtering();
    test_json_formatting();
    printf("All logging tests passed.\n");
}
