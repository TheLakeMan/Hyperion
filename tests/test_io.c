/**
 * Hyperion I/O System Tests
 */

#include "test_framework.h"
#include "../core/io.h"

#include <stdio.h>
#include <string.h>

static void makeTempPath(char *buffer, size_t size, const char *hint)
{
    static unsigned counter = 0;
    snprintf(buffer, size, "hyperion_test_%s_%u.tmp", hint, counter++);
}

static void cleanupFile(const char *path)
{
    if (path) {
        hyperionDeleteFile(path);
    }
}

static void cleanupDir(const char *path)
{
    if (path) {
        hyperionDeleteDir(path, 0);
    }
}

HYPERION_TEST(test_io_file_operations)
{
    HYPERION_ASSERT(hyperionIOInit() == 0, "hyperionIOInit should succeed");

    char filePath[128];
    makeTempPath(filePath, sizeof(filePath), "file_ops");
    const char *testContent = "Hello Hyperion I/O!\nLine 2.";

    cleanupFile(filePath);

    HyperionFile *file = hyperionOpenFile(filePath, HYPERION_FILE_WRITE | HYPERION_FILE_TRUNCATE |
                                                      HYPERION_FILE_CREATE);
    HYPERION_ASSERT(file != NULL, "Failed to create temp file");
    HYPERION_ASSERT(hyperionWriteFile(file, testContent, strlen(testContent)) ==
                        (int64_t)strlen(testContent),
                    "Failed to write test content");
    hyperionCloseFile(file);

    HYPERION_ASSERT(hyperionFileExists(filePath) == 1, "Temp file should exist");

    file = hyperionOpenFile(filePath, HYPERION_FILE_READ);
    HYPERION_ASSERT(file != NULL, "Failed to reopen temp file for reading");

    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    int64_t bytesRead = hyperionReadFile(file, buffer, sizeof(buffer) - 1);
    HYPERION_ASSERT(bytesRead == (int64_t)strlen(testContent),
                    "Read length should match written content");
    HYPERION_ASSERT(strcmp(buffer, testContent) == 0, "Read content mismatch");

    memset(buffer, 0, sizeof(buffer));
    bytesRead = hyperionReadFile(file, buffer, 1);
    HYPERION_ASSERT(bytesRead == 0, "Reading past end should return zero");
    HYPERION_ASSERT(hyperionEOF(file) == 1, "EOF should be signaled after read past end");

    hyperionCloseFile(file);

    HYPERION_ASSERT(hyperionDeleteFile(filePath) == HYPERION_IO_SUCCESS,
                    "Deleting temp file should succeed");
    HYPERION_ASSERT(hyperionFileExists(filePath) == 0, "Temp file should be removed");

    hyperionIOCleanup();

    return 0;
}

HYPERION_TEST(test_io_file_modes)
{
    HYPERION_ASSERT(hyperionIOInit() == 0, "hyperionIOInit should succeed");

    char filePath[128];
    makeTempPath(filePath, sizeof(filePath), "file_modes");

    cleanupFile(filePath);

    HyperionFile *file = hyperionOpenFile(filePath, HYPERION_FILE_WRITE | HYPERION_FILE_TRUNCATE |
                                                      HYPERION_FILE_CREATE);
    HYPERION_ASSERT(file != NULL, "Failed to create file for mode tests");
    const char *initialContent = "Initial";
    HYPERION_ASSERT(hyperionWriteFile(file, initialContent, strlen(initialContent)) ==
                        (int64_t)strlen(initialContent),
                    "Initial write failed");
    hyperionCloseFile(file);

    file = hyperionOpenFile(filePath, HYPERION_FILE_APPEND);
    HYPERION_ASSERT(file != NULL, "Failed to open file for append");
    const char *appendContent = " Appended";
    HYPERION_ASSERT(hyperionWriteFile(file, appendContent, strlen(appendContent)) ==
                        (int64_t)strlen(appendContent),
                    "Append write failed");
    hyperionCloseFile(file);

    file = hyperionOpenFile(filePath, HYPERION_FILE_READ);
    HYPERION_ASSERT(file != NULL, "Failed to open file for read after append");
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    HYPERION_ASSERT(hyperionReadFile(file, buffer, sizeof(buffer) - 1) > 0,
                    "Read after append failed");
    HYPERION_ASSERT(strcmp(buffer, "Initial Appended") == 0, "Append content mismatch");
    hyperionCloseFile(file);

    file = hyperionOpenFile(filePath, HYPERION_FILE_WRITE | HYPERION_FILE_TRUNCATE);
    HYPERION_ASSERT(file != NULL, "Failed to open file for truncate");
    const char *truncateContent = "Truncated";
    HYPERION_ASSERT(hyperionWriteFile(file, truncateContent, strlen(truncateContent)) ==
                        (int64_t)strlen(truncateContent),
                    "Truncate write failed");
    hyperionCloseFile(file);

    file = hyperionOpenFile(filePath, HYPERION_FILE_READ);
    HYPERION_ASSERT(file != NULL, "Failed to open file for read after truncate");
    memset(buffer, 0, sizeof(buffer));
    HYPERION_ASSERT(hyperionReadFile(file, buffer, sizeof(buffer) - 1) > 0,
                    "Read after truncate failed");
    HYPERION_ASSERT(strcmp(buffer, truncateContent) == 0, "Truncate content mismatch");
    hyperionCloseFile(file);

    cleanupFile(filePath);

    hyperionIOCleanup();

    return 0;
}

HYPERION_TEST(test_io_directory_operations)
{
    HYPERION_ASSERT(hyperionIOInit() == 0, "hyperionIOInit should succeed");

    char dirPath[128];
    makeTempPath(dirPath, sizeof(dirPath), "dir_ops");

    cleanupDir(dirPath);

    HYPERION_ASSERT(hyperionCreateDir(dirPath) == HYPERION_IO_SUCCESS,
                    "Creating directory should succeed");

    HyperionFileInfo info;
    memset(&info, 0, sizeof(info));
    HYPERION_ASSERT(hyperionGetFileInfo(dirPath, &info) == HYPERION_IO_SUCCESS,
                    "Directory info lookup failed");
    HYPERION_ASSERT(info.isDirectory == 1, "Created path should be a directory");
    hyperionFreeFileInfo(&info);

    HYPERION_ASSERT(hyperionDeleteDir(dirPath, 0) == HYPERION_IO_SUCCESS,
                    "Deleting directory should succeed");
    HYPERION_ASSERT(hyperionFileExists(dirPath) == 0,
                    "Directory should not exist after deletion");

    hyperionIOCleanup();

    return 0;
}

const HyperionTestCase g_io_tests[] = {
    {"io_file_operations", "io", test_io_file_operations},
    {"io_file_modes", "io", test_io_file_modes},
    {"io_directory_operations", "io", test_io_directory_operations},
};

const size_t g_io_test_count = sizeof(g_io_tests) / sizeof(g_io_tests[0]);
