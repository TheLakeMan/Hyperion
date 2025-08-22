/**
 * Hyperion I/O System Header
 * 
 * This header defines the I/O system for Hyperion, providing cross-platform
 * file operations for loading models and saving configurations.
 */

#ifndef HYPERION_IO_H
#define HYPERION_IO_H

#include <stddef.h>
#include <stdint.h>

/* ----------------- File Mode Flags ----------------- */

#define HYPERION_FILE_READ        0x01    /* Open for reading */
#define HYPERION_FILE_WRITE       0x02    /* Open for writing */
#define HYPERION_FILE_APPEND      0x04    /* Open for appending */
#define HYPERION_FILE_BINARY      0x08    /* Open in binary mode */
#define HYPERION_FILE_CREATE      0x10    /* Create if doesn't exist */
#define HYPERION_FILE_TRUNCATE    0x20    /* Truncate if exists */

/* ----------------- File Error Codes ----------------- */

#define HYPERION_IO_SUCCESS       0       /* Operation successful */
#define HYPERION_IO_ERROR        -1       /* Generic I/O error */
#define HYPERION_IO_NOT_FOUND    -2       /* File not found */
#define HYPERION_IO_ACCESS       -3       /* Permission denied */
#define HYPERION_IO_EXISTS       -4       /* File already exists */
#define HYPERION_IO_INVALID      -5       /* Invalid argument */
#define HYPERION_IO_NO_MEMORY    -6       /* Out of memory */
#define HYPERION_IO_EOF          -7       /* End of file */

/* ----------------- Type Definitions ----------------- */

/* File handle type */
typedef struct HyperionFile HyperionFile;

/* Directory handle type */
typedef struct HyperionDir HyperionDir;

/* File information structure */
typedef struct {
    char *path;             /* Full path to file */
    uint64_t size;          /* File size in bytes */
    uint64_t modTime;       /* Last modification time */
    uint32_t mode;          /* File mode */
    int isDirectory;        /* Whether it's a directory */
} HyperionFileInfo;

/* ----------------- System Functions ----------------- */

/**
 * Initialize I/O system
 * 
 * @return 0 on success, non-zero on error
 */
int hyperionIOInit();

/**
 * Clean up I/O system
 */
void hyperionIOCleanup();

/**
 * Get last I/O error code
 * 
 * @return Last error code
 */
int hyperionIOGetLastError();

/**
 * Get error message for error code
 * 
 * @param error Error code
 * @return Error message string
 */
const char* hyperionIOGetErrorString(int error);

/* ----------------- File Operations ----------------- */

/**
 * Open a file
 * 
 * @param path File path
 * @param mode File mode flags
 * @return File handle or NULL on error
 */
HyperionFile* hyperionOpenFile(const char *path, int mode);

/**
 * Close a file
 * 
 * @param file File handle
 */
void hyperionCloseFile(HyperionFile *file);

/**
 * Read from a file
 * 
 * @param file File handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes read or negative error code
 */
int64_t hyperionReadFile(HyperionFile *file, void *buffer, size_t size);

/**
 * Write to a file
 * 
 * @param file File handle
 * @param buffer Buffer to write from
 * @param size Number of bytes to write
 * @return Number of bytes written or negative error code
 */
int64_t hyperionWriteFile(HyperionFile *file, const void *buffer, size_t size);

/**
 * Read a line from a file
 * 
 * @param file File handle
 * @param buffer Buffer to read into
 * @param size Buffer size
 * @return Number of bytes read or negative error code
 */
int hyperionReadLine(HyperionFile *file, char *buffer, size_t size);

/**
 * Seek to position in file
 * 
 * @param file File handle
 * @param offset Offset in bytes
 * @param whence Seek origin (0=start, 1=current, 2=end)
 * @return New position or negative error code
 */
int64_t hyperionSeekFile(HyperionFile *file, int64_t offset, int whence);

/**
 * Get current position in file
 * 
 * @param file File handle
 * @return Current position or negative error code
 */
int64_t hyperionTellFile(HyperionFile *file);

/**
 * Flush file buffers
 * 
 * @param file File handle
 * @return 0 on success, negative error code on failure
 */
int hyperionFlushFile(HyperionFile *file);

/**
 * Check if end of file
 * 
 * @param file File handle
 * @return 1 if EOF, 0 if not, negative error code on failure
 */
int hyperionEOF(HyperionFile *file);

/* ----------------- File System Operations ----------------- */

/**
 * Check if file exists
 * 
 * @param path File path
 * @return 1 if exists, 0 if not, negative error code on failure
 */
int hyperionFileExists(const char *path);

/**
 * Delete a file
 * 
 * @param path File path
 * @return 0 on success, negative error code on failure
 */
int hyperionDeleteFile(const char *path);

/**
 * Rename a file
 * 
 * @param oldPath Old file path
 * @param newPath New file path
 * @return 0 on success, negative error code on failure
 */
int hyperionRenameFile(const char *oldPath, const char *newPath);

/**
 * Get file information
 * 
 * @param path File path
 * @param info File information structure
 * @return 0 on success, negative error code on failure
 */
int hyperionGetFileInfo(const char *path, HyperionFileInfo *info);

/**
 * Free file information structure
 * 
 * @param info File information structure
 */
void hyperionFreeFileInfo(HyperionFileInfo *info);

/* ----------------- Directory Operations ----------------- */

/**
 * Create a directory
 * 
 * @param path Directory path
 * @return 0 on success, negative error code on failure
 */
int hyperionCreateDir(const char *path);

/**
 * Delete a directory
 * 
 * @param path Directory path
 * @param recursive Whether to delete recursively
 * @return 0 on success, negative error code on failure
 */
int hyperionDeleteDir(const char *path, int recursive);

/**
 * Open a directory for reading
 * 
 * @param path Directory path
 * @return Directory handle or NULL on error
 */
HyperionDir* hyperionOpenDir(const char *path);

/**
 * Close a directory
 * 
 * @param dir Directory handle
 */
void hyperionCloseDir(HyperionDir *dir);

/**
 * Read next directory entry
 * 
 * @param dir Directory handle
 * @param name Buffer to store entry name
 * @param size Buffer size
 * @return 1 if entry read, 0 if no more entries, negative error code on failure
 */
int hyperionReadDir(HyperionDir *dir, char *name, size_t size);

/**
 * Get current working directory
 * 
 * @param buffer Buffer to store path
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int hyperionGetCWD(char *buffer, size_t size);

/**
 * Change current working directory
 * 
 * @param path New directory path
 * 
 * @return 0 on success, negative error code on failure
 */
int hyperionSetCWD(const char *path);

/* ----------------- Path Operations ----------------- */

/**
 * Get path separator
 * 
 * @return Path separator character ('/' or '\\')
 */
char hyperionGetPathSeparator();

/**
 * Join path components
 * 
 * @param buffer Buffer to store result
 * @param size Buffer size
 * @param count Number of components
 * @param ... Path components
 * @return 0 on success, negative error code on failure
 */
int hyperionJoinPath(char *buffer, size_t size, int count, ...);

/**
 * Get file name from path
 * 
 * @param path File path
 * @return File name
 */
const char* hyperionGetFileName(const char *path);

/**
 * Get file extension from path
 * 
 * @param path File path
 * @return File extension
 */
const char* hyperionGetFileExt(const char *path);

/**
 * Get directory name from path
 * 
 * @param path File path
 * @param buffer Buffer to store result
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int hyperionGetDirName(const char *path, char *buffer, size_t size);

/**
 * Get absolute path
 * 
 * @param path Path to resolve
 * @param buffer Buffer to store result
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int hyperionGetAbsPath(const char *path, char *buffer, size_t size);

#endif /* HYPERION_IO_H */
