/**
 * Hyperion I/O System Tests
 */

#include "../core/io.h" // Include the header for the functions being tested
#include <stdio.h>
#include <stdlib.h> // For exit()
#include <string.h> // For strcmp, etc.

// Basic assertion helper (could be moved to a common test header)
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion Failed: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            fprintf(stderr, "  Last IO Error: %d (%s)\n", hyperionIOGetLastError(), hyperionIOGetErrorString(hyperionIOGetLastError())); \
            exit(1); \
        } \
    } while (0)

// Helper to create a temporary test file
const char* create_temp_file(const char* name, const char* content) {
    HyperionFile* f = hyperionOpenFile(name, TINYAI_FILE_WRITE | TINYAI_FILE_TRUNCATE);
    if (!f) {
        fprintf(stderr, "Failed to create temp file '%s' for testing.\n", name);
        exit(1);
    }
    if (content) {
        hyperionWriteFile(f, content, strlen(content));
    }
    hyperionCloseFile(f);
    return name;
}

// Helper to delete a temporary test file
void delete_temp_file(const char* name) {
    hyperionDeleteFile(name);
}

void test_file_operations() {
    printf("  Testing basic file operations (create, write, read, delete)...\n");
    const char* testFileName = "hyperion_test_io_temp.txt";
    const char* testContent = "Hello Hyperion I/O!\nLine 2.";
    
    // Create and Write
    create_temp_file(testFileName, testContent);
    ASSERT(hyperionFileExists(testFileName) == 1, "Temp file should exist after creation.");

    // Read back
    HyperionFile* f = hyperionOpenFile(testFileName, TINYAI_FILE_READ);
    ASSERT(f != NULL, "Should be able to open temp file for reading.");
    
    char buffer[100];
    memset(buffer, 0, sizeof(buffer));
    int64_t bytesRead = hyperionReadFile(f, buffer, sizeof(buffer) - 1);
    
    ASSERT(bytesRead == (int64_t)strlen(testContent), "Bytes read should match content length.");
    ASSERT(strcmp(buffer, testContent) == 0, "Read content should match written content.");

    // Test EOF - Reading the exact amount might set EOF, so the robust check
    // is trying to read *past* the end.
    // ASSERT(hyperionEOF(f) == 0, "Should not be EOF yet."); // Removed this potentially flaky check
    bytesRead = hyperionReadFile(f, buffer, 1); // Try reading one more byte
    ASSERT(bytesRead == 0, "Reading past end should return 0 bytes.");
    ASSERT(hyperionEOF(f) == 1, "Should be EOF after reading past end.");
    
    hyperionCloseFile(f);

    // Delete
    int delResult = hyperionDeleteFile(testFileName);
    ASSERT(delResult == TINYAI_IO_SUCCESS, "Deleting temp file should succeed.");
    ASSERT(hyperionFileExists(testFileName) == 0, "Temp file should not exist after deletion.");

    printf("    PASS\n");
}

void test_file_modes() {
    printf("  Testing file open modes...\n");
    const char* testFileName = "hyperion_test_modes_temp.txt";
    const char* initialContent = "Initial";
    const char* appendContent = " Appended";

    // Write initial content
    create_temp_file(testFileName, initialContent);

    // Test Append
    HyperionFile* f = hyperionOpenFile(testFileName, TINYAI_FILE_APPEND);
    ASSERT(f != NULL, "Should open for append.");
    hyperionWriteFile(f, appendContent, strlen(appendContent));
    hyperionCloseFile(f);

    // Verify append
    f = hyperionOpenFile(testFileName, TINYAI_FILE_READ);
    ASSERT(f != NULL, "Should open for read after append.");
    char buffer[100];
    memset(buffer, 0, sizeof(buffer));
    hyperionReadFile(f, buffer, sizeof(buffer) - 1);
    const char* expectedAppend = "Initial Appended";
    ASSERT(strcmp(buffer, expectedAppend) == 0, "Append content should be correct.");
    hyperionCloseFile(f);

    // Test Truncate (via TINYAI_FILE_WRITE)
    f = hyperionOpenFile(testFileName, TINYAI_FILE_WRITE); // Default write truncates
    ASSERT(f != NULL, "Should open for write (truncate).");
    const char* truncateContent = "Truncated";
    hyperionWriteFile(f, truncateContent, strlen(truncateContent));
    hyperionCloseFile(f);

    // Verify truncate
    f = hyperionOpenFile(testFileName, TINYAI_FILE_READ);
    ASSERT(f != NULL, "Should open for read after truncate.");
    memset(buffer, 0, sizeof(buffer));
    hyperionReadFile(f, buffer, sizeof(buffer) - 1);
    ASSERT(strcmp(buffer, truncateContent) == 0, "Truncated content should be correct.");
    hyperionCloseFile(f);

    delete_temp_file(testFileName);
    printf("    PASS\n");
}

void test_directory_operations() {
     printf("  Testing basic directory operations (create, delete)...\n");
     const char* testDirName = "hyperion_test_dir_temp";

     // Ensure doesn't exist initially
     hyperionDeleteDir(testDirName, 0); // Ignore error if it doesn't exist

     // Create
     int createResult = hyperionCreateDir(testDirName);
     ASSERT(createResult == TINYAI_IO_SUCCESS, "Creating directory should succeed.");
     
     // Check existence (using file info)
     HyperionFileInfo info;
     int infoResult = hyperionGetFileInfo(testDirName, &info);
     ASSERT(infoResult == TINYAI_IO_SUCCESS, "Getting info for created directory should succeed.");
     ASSERT(info.isDirectory == 1, "Created path should be a directory.");
     hyperionFreeFileInfo(&info); // Clean up allocated path in info struct

     // Delete
     int deleteResult = hyperionDeleteDir(testDirName, 0); // Non-recursive
     ASSERT(deleteResult == TINYAI_IO_SUCCESS, "Deleting empty directory should succeed.");
     
     // Verify deletion
     ASSERT(hyperionFileExists(testDirName) == 0, "Directory should not exist after deletion.");

     printf("    PASS\n");
}

// Function to be called by test_main.c
void run_io_tests() {
    printf("--- Running I/O Tests ---\n");
    
    // Initialize IO system for tests (if needed, though likely simple)
    hyperionIOInit(); 

    test_file_operations();
    test_file_modes();
    test_directory_operations();
    // Add calls to path tests, directory listing tests, etc.

    hyperionIOCleanup(); // Cleanup IO system

    printf("--- I/O Tests Finished ---\n");
}
