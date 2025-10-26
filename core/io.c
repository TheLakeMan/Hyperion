/**
 * Hyperion I/O System Implementation
 */

#include "io.h"
#include "memory.h" // Use Hyperion memory functions if available/needed

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h> // For stat, mkdir
#include <sys/types.h>
#include <stdarg.h> // For va_list in hyperionJoinPath

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <direct.h> // For _mkdir, _rmdir, _getcwd, _chdir
#define stat _stat
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define getcwd _getcwd
#define chdir _chdir
#define mkdir(path, mode) _mkdir(path) // mode is ignored on Windows _mkdir
#define rmdir _rmdir
#define PATH_SEPARATOR '\\'
#else // POSIX
#include <unistd.h> // For rmdir, getcwd, chdir, access
#include <dirent.h> // For opendir, readdir, closedir
#define PATH_SEPARATOR '/'
#endif

/* ----------------- Internal State & Error Handling ----------------- */

static int g_lastIOError = HYPERION_IO_SUCCESS;

// Helper to map errno to Hyperion error codes
static int mapErrnoToHyperionError(int err) {
    switch (err) {
        case 0: return HYPERION_IO_SUCCESS;
        case ENOENT: return HYPERION_IO_NOT_FOUND;
        case EACCES: return HYPERION_IO_ACCESS;
        case EEXIST: return HYPERION_IO_EXISTS;
        case EINVAL: return HYPERION_IO_INVALID;
        case ENOMEM: return HYPERION_IO_NO_MEMORY;
        // Add more mappings as needed
        default: return HYPERION_IO_ERROR; // Generic error
    }
}

// Helper to set the last error
static void setLastError(int hyperionError) {
    g_lastIOError = hyperionError;
}

static void setLastErrorFromErrno() {
    g_lastIOError = mapErrnoToHyperionError(errno);
}

/* ----------------- Type Definitions ----------------- */

// File handle structure wraps standard FILE*
struct HyperionFile {
    FILE *fp;
    int mode; // Store the mode flags used to open
};

// Directory handle structure (Platform-dependent)
#ifdef _WIN32
struct HyperionDir {
    HANDLE hFind;
    WIN32_FIND_DATA findData;
    char *path; // Store path for subsequent FindNextFile calls
    int firstEntry; // Flag to handle the first FindFirstFile call
};
#else // POSIX
struct HyperionDir {
    DIR *dp;
    struct dirent *entry;
};
#endif


/* ----------------- System Functions ----------------- */

int hyperionIOInit() {
    // Nothing specific to initialize for basic stdio usage
    g_lastIOError = HYPERION_IO_SUCCESS;
    return 0;
}

void hyperionIOCleanup() {
    // Nothing specific to clean up
}

int hyperionIOGetLastError() {
    return g_lastIOError;
}

const char* hyperionIOGetErrorString(int error) {
    switch (error) {
        case HYPERION_IO_SUCCESS: return "Success";
        case HYPERION_IO_ERROR: return "Generic I/O error";
        case HYPERION_IO_NOT_FOUND: return "File not found";
        case HYPERION_IO_ACCESS: return "Permission denied";
        case HYPERION_IO_EXISTS: return "File already exists";
        case HYPERION_IO_INVALID: return "Invalid argument or operation";
        case HYPERION_IO_NO_MEMORY: return "Out of memory";
        case HYPERION_IO_EOF: return "End of file";
        default: return "Unknown error code";
    }
}

/* ----------------- File Operations ----------------- */

HyperionFile* hyperionOpenFile(const char *path, int mode) {
    char fopen_mode[5] = {0}; // Max "rb+" + null terminator
    int current_pos = 0;

    if (mode & HYPERION_FILE_READ && mode & HYPERION_FILE_WRITE) {
        fopen_mode[current_pos++] = 'r';
        fopen_mode[current_pos++] = '+';
    } else if (mode & HYPERION_FILE_WRITE) {
        fopen_mode[current_pos++] = 'w';
         if (mode & HYPERION_FILE_READ) fopen_mode[current_pos++] = '+';
    } else if (mode & HYPERION_FILE_APPEND) {
        fopen_mode[current_pos++] = 'a';
         if (mode & HYPERION_FILE_READ) fopen_mode[current_pos++] = '+';
    } else if (mode & HYPERION_FILE_READ) {
        fopen_mode[current_pos++] = 'r';
    } else {
        setLastError(HYPERION_IO_INVALID); // No read/write/append specified
        return NULL;
    }

    if (mode & HYPERION_FILE_BINARY) {
        fopen_mode[current_pos++] = 'b';
    }
    
    // Note: HYPERION_FILE_CREATE and HYPERION_FILE_TRUNCATE are implicitly handled by 'w' and 'a' modes.
    // 'w' creates or truncates. 'a' creates or appends. 'r+' requires file to exist.
    // More complex logic could be added if exact POSIX open() flag behavior is needed.

    FILE *fp = fopen(path, fopen_mode);
    if (!fp) {
        setLastErrorFromErrno();
        return NULL;
    }

    // Allocate HyperionFile structure
    // Use HYPERION_MALLOC if memory tracking is enabled and integrated
    HyperionFile *taiFile = (HyperionFile*)malloc(sizeof(HyperionFile)); 
    if (!taiFile) {
        fclose(fp);
        setLastError(HYPERION_IO_NO_MEMORY);
        return NULL;
    }

    taiFile->fp = fp;
    taiFile->mode = mode;
    setLastError(HYPERION_IO_SUCCESS);
    return taiFile;
}

void hyperionCloseFile(HyperionFile *file) {
    if (file && file->fp) {
        fclose(file->fp);
        // Use HYPERION_FREE if memory tracking is enabled and integrated
        free(file); 
    }
}

int64_t hyperionReadFile(HyperionFile *file, void *buffer, size_t size) {
    if (!file || !file->fp || !buffer) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }

    size_t bytesRead = fread(buffer, 1, size, file->fp);
    if (bytesRead < size) {
        if (feof(file->fp)) {
            // Reached EOF, return bytes read, set EOF state if bytesRead is 0?
            // Standard fread returns partial read on EOF.
             setLastError(HYPERION_IO_EOF); // Indicate EOF was reached
             // return bytesRead; // Return actual bytes read before EOF
        } else if (ferror(file->fp)) {
            setLastErrorFromErrno(); // Or just HYPERION_IO_ERROR
            return g_lastIOError;
        }
        // If bytesRead < size and not EOF and not error, it's unexpected.
    } else {
         setLastError(HYPERION_IO_SUCCESS);
    }
    return (int64_t)bytesRead;
}

int64_t hyperionWriteFile(HyperionFile *file, const void *buffer, size_t size) {
     if (!file || !file->fp || !buffer) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }
     if (!(file->mode & (HYPERION_FILE_WRITE | HYPERION_FILE_APPEND))) {
         setLastError(HYPERION_IO_ACCESS); // Opened read-only
         return HYPERION_IO_ACCESS;
     }

    size_t bytesWritten = fwrite(buffer, 1, size, file->fp);
     if (bytesWritten < size) {
         // Error occurred
         setLastErrorFromErrno(); // Or just HYPERION_IO_ERROR
         return g_lastIOError;
     }
    
    setLastError(HYPERION_IO_SUCCESS);
    return (int64_t)bytesWritten;
}

int hyperionReadLine(HyperionFile *file, char *buffer, size_t size) {
    if (!file || !file->fp || !buffer || size == 0) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }

    char *result = fgets(buffer, (int)size, file->fp);
    if (result == NULL) {
        if (feof(file->fp)) {
            setLastError(HYPERION_IO_EOF);
            buffer[0] = '\0'; // Ensure buffer is empty on EOF
            return HYPERION_IO_EOF; // Return specific EOF code
        } else {
            setLastErrorFromErrno(); // Or HYPERION_IO_ERROR
            return g_lastIOError;
        }
    }
    
    // Remove trailing newline if present and buffer has space
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
         if (len > 0 && buffer[len - 1] == '\r') { // Handle CRLF
             buffer[len - 1] = '\0';
             len--;
         }
    }

    setLastError(HYPERION_IO_SUCCESS);
    return (int)len; // Return number of bytes read (excluding null terminator)
}


int64_t hyperionSeekFile(HyperionFile *file, int64_t offset, int whence) {
    if (!file || !file->fp) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }

    int fseek_whence;
    switch (whence) {
        case 0: fseek_whence = SEEK_SET; break;
        case 1: fseek_whence = SEEK_CUR; break;
        case 2: fseek_whence = SEEK_END; break;
        default:
            setLastError(HYPERION_IO_INVALID);
            return HYPERION_IO_INVALID;
    }

    // fseek returns 0 on success
    if (fseek(file->fp, (long)offset, fseek_whence) != 0) {
        setLastErrorFromErrno(); // Or HYPERION_IO_ERROR
        return g_lastIOError;
    }

    // Return the new position after seeking
    return hyperionTellFile(file);
}

int64_t hyperionTellFile(HyperionFile *file) {
    if (!file || !file->fp) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }
    long pos = ftell(file->fp);
    if (pos == -1L) {
        setLastErrorFromErrno(); // Or HYPERION_IO_ERROR
        return g_lastIOError;
    }
    setLastError(HYPERION_IO_SUCCESS);
    return (int64_t)pos;
}

int hyperionFlushFile(HyperionFile *file) {
    if (!file || !file->fp) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }
    if (fflush(file->fp) != 0) {
        setLastErrorFromErrno(); // Or HYPERION_IO_ERROR
        return g_lastIOError;
    }
    setLastError(HYPERION_IO_SUCCESS);
    return HYPERION_IO_SUCCESS;
}

int hyperionEOF(HyperionFile *file) {
     if (!file || !file->fp) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID; // Indicate error
    }
    int eof_status = feof(file->fp);
    setLastError(HYPERION_IO_SUCCESS); // Checking EOF is not an error itself
    return eof_status ? 1 : 0; // Return 1 if EOF, 0 otherwise
}


/* ----------------- File System Operations ----------------- */

int hyperionFileExists(const char *path) {
    struct stat buffer;
    if (stat(path, &buffer) == 0) {
        setLastError(HYPERION_IO_SUCCESS);
        return 1; // Exists
    } else {
        if (errno == ENOENT) {
            setLastError(HYPERION_IO_NOT_FOUND);
            return 0; // Does not exist
        } else {
            setLastErrorFromErrno(); // Other error (e.g., permission)
            return g_lastIOError; // Return error code
        }
    }
}

int hyperionDeleteFile(const char *path) {
    if (remove(path) == 0) {
        setLastError(HYPERION_IO_SUCCESS);
        return HYPERION_IO_SUCCESS;
    } else {
        setLastErrorFromErrno();
        return g_lastIOError;
    }
}

int hyperionRenameFile(const char *oldPath, const char *newPath) {
    if (rename(oldPath, newPath) == 0) {
        setLastError(HYPERION_IO_SUCCESS);
        return HYPERION_IO_SUCCESS;
    } else {
        setLastErrorFromErrno();
        return g_lastIOError;
    }
}

int hyperionGetFileInfo(const char *path, HyperionFileInfo *info) {
    if (!path || !info) {
         setLastError(HYPERION_IO_INVALID);
         return HYPERION_IO_INVALID;
    }
    
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        setLastErrorFromErrno();
        return g_lastIOError;
    }

    // Allocate memory for the path copy
    // Use HYPERION_MALLOC if integrated
    info->path = (char*)malloc(strlen(path) + 1); 
    if (!info->path) {
        setLastError(HYPERION_IO_NO_MEMORY);
        return HYPERION_IO_NO_MEMORY;
    }
    strcpy(info->path, path);

    info->size = (uint64_t)statbuf.st_size;
    info->modTime = (uint64_t)statbuf.st_mtime;
    info->mode = (uint32_t)statbuf.st_mode; // Keep platform mode bits
    info->isDirectory = S_ISDIR(statbuf.st_mode);

    setLastError(HYPERION_IO_SUCCESS);
    return HYPERION_IO_SUCCESS;
}

void hyperionFreeFileInfo(HyperionFileInfo *info) {
    if (info && info->path) {
        // Use HYPERION_FREE if integrated
        free(info->path); 
        info->path = NULL; // Prevent double free
    }
    // No need to free the struct itself if allocated on stack
}


/* ----------------- Directory Operations (Basic POSIX/Windows Stubs) ----------------- */

int hyperionCreateDir(const char *path) {
    // Note: mode 0777 is common for POSIX, ignored by Windows _mkdir
    if (mkdir(path, 0777) == 0) {
        setLastError(HYPERION_IO_SUCCESS);
        return HYPERION_IO_SUCCESS;
    } else {
        // Check if it already exists (EEXIST is often success for mkdir)
        if (errno == EEXIST) {
             // Verify it's actually a directory
             struct stat st;
             if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                 setLastError(HYPERION_IO_SUCCESS); // Treat existing dir as success
                 return HYPERION_IO_SUCCESS;
             }
        }
        setLastErrorFromErrno();
        return g_lastIOError;
    }
}

int hyperionDeleteDir(const char *path, int recursive) {
    // Recursive delete is complex and platform-specific.
    // Provide non-recursive only for now.
    if (recursive) {
        fprintf(stderr, "Warning: Recursive directory delete not yet implemented in hyperionDeleteDir.\n");
        setLastError(HYPERION_IO_INVALID); // Or a specific "not implemented" error
        return HYPERION_IO_INVALID;
    }

    if (rmdir(path) == 0) {
        setLastError(HYPERION_IO_SUCCESS);
        return HYPERION_IO_SUCCESS;
    } else {
        setLastErrorFromErrno();
        return g_lastIOError;
    }
}

HyperionDir* hyperionOpenDir(const char *path) {
#ifdef _WIN32
    // Windows implementation using FindFirstFile/FindNextFile
    HyperionDir *dir = (HyperionDir*)malloc(sizeof(HyperionDir));
    if (!dir) { setLastError(HYPERION_IO_NO_MEMORY); return NULL; }

    size_t pathLen = strlen(path);
    // Append \*.* for FindFirstFile
    dir->path = (char*)malloc(pathLen + 5); // path + "\\*.*" + null
    if (!dir->path) { free(dir); setLastError(HYPERION_IO_NO_MEMORY); return NULL; }
    strcpy(dir->path, path);
    // Ensure trailing separator
    if (pathLen > 0 && dir->path[pathLen - 1] != PATH_SEPARATOR) {
        dir->path[pathLen] = PATH_SEPARATOR;
        dir->path[pathLen + 1] = '\0';
    }
    strcat(dir->path, "*.*");

    dir->hFind = FindFirstFile(dir->path, &dir->findData);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        free(dir->path);
        free(dir);
        setLastErrorFromErrno(); // Map GetLastError() if needed
        return NULL;
    }
    dir->firstEntry = 1;
    setLastError(HYPERION_IO_SUCCESS);
    return dir;
#else // POSIX
    DIR *dp = opendir(path);
    if (!dp) {
        setLastErrorFromErrno();
        return NULL;
    }
    HyperionDir *dir = (HyperionDir*)malloc(sizeof(HyperionDir));
     if (!dir) { 
         closedir(dp); 
         setLastError(HYPERION_IO_NO_MEMORY); 
         return NULL; 
     }
    dir->dp = dp;
    dir->entry = NULL;
    setLastError(HYPERION_IO_SUCCESS);
    return dir;
#endif
}

void hyperionCloseDir(HyperionDir *dir) {
    if (!dir) return;
#ifdef _WIN32
    if (dir->hFind != INVALID_HANDLE_VALUE) {
        FindClose(dir->hFind);
    }
    free(dir->path);
    free(dir);
#else // POSIX
    if (dir->dp) {
        closedir(dir->dp);
    }
    free(dir);
#endif
}

int hyperionReadDir(HyperionDir *dir, char *name, size_t size) {
     if (!dir || !name || size == 0) {
         setLastError(HYPERION_IO_INVALID);
         return HYPERION_IO_INVALID;
     }
#ifdef _WIN32
    if (dir->hFind == INVALID_HANDLE_VALUE) {
         setLastError(HYPERION_IO_INVALID);
         return HYPERION_IO_INVALID;
    }

    // Handle the first entry from FindFirstFile
    if (dir->firstEntry) {
        dir->firstEntry = 0; // Consume the first entry
         // Skip "." and ".." directories
        while (strcmp(dir->findData.cFileName, ".") == 0 || strcmp(dir->findData.cFileName, "..") == 0) {
             if (!FindNextFile(dir->hFind, &dir->findData)) {
                 if (GetLastError() == ERROR_NO_MORE_FILES) {
                     setLastError(HYPERION_IO_SUCCESS);
                     return 0;
                 } else {
                     setLastErrorFromErrno();
                     return g_lastIOError;
                 }
             }
        }
        strncpy(name, dir->findData.cFileName, size - 1);
        name[size - 1] = '\0';
        setLastError(HYPERION_IO_SUCCESS);
        return 1;
    }

    // Subsequent entries using FindNextFile
    while (FindNextFile(dir->hFind, &dir->findData)) {
         // Skip "." and ".." directories
         if (strcmp(dir->findData.cFileName, ".") != 0 && strcmp(dir->findData.cFileName, "..") != 0) {
             strncpy(name, dir->findData.cFileName, size - 1);
             name[size - 1] = '\0';
             setLastError(HYPERION_IO_SUCCESS);
             return 1;
         }
    }

    // Check why FindNextFile failed
    if (GetLastError() == ERROR_NO_MORE_FILES) {
        setLastError(HYPERION_IO_SUCCESS);
        return 0;
    } else {
        setLastErrorFromErrno();
        return g_lastIOError;
    }
#else // POSIX
    if (!dir->dp) {
         setLastError(HYPERION_IO_INVALID);
         return HYPERION_IO_INVALID;
    }
    
    errno = 0;
    while ((dir->entry = readdir(dir->dp)) != NULL) {
        // Skip "." and ".."
        if (strcmp(dir->entry->d_name, ".") == 0 || strcmp(dir->entry->d_name, "..") == 0) {
            continue;
        }
        strncpy(name, dir->entry->d_name, size - 1);
        name[size - 1] = '\0';
        setLastError(HYPERION_IO_SUCCESS);
        return 1;
    }

    // Check if readdir finished or encountered an error
    if (errno != 0) {
        setLastErrorFromErrno();
        return g_lastIOError;
    } else {
        setLastError(HYPERION_IO_SUCCESS);
        return 0;
    }
#endif
}


int hyperionGetCWD(char *buffer, size_t size) {
    if (getcwd(buffer, size) != NULL) {
        setLastError(HYPERION_IO_SUCCESS);
        return HYPERION_IO_SUCCESS;
    } else {
        setLastErrorFromErrno();
        return g_lastIOError;
    }
}

int hyperionSetCWD(const char *path) {
    if (chdir(path) == 0) {
        setLastError(HYPERION_IO_SUCCESS);
        return HYPERION_IO_SUCCESS;
    } else {
        setLastErrorFromErrno();
        return g_lastIOError;
    }
}

/* ----------------- Path Operations (Basic Implementations) ----------------- */

char hyperionGetPathSeparator() {
    return PATH_SEPARATOR;
}

int hyperionJoinPath(char *buffer, size_t size, int count, ...) {
    if (!buffer || size == 0 || count <= 0) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }

    va_list args;
    va_start(args, count);

    buffer[0] = '\0';
    size_t currentLen = 0;
    char separator[2] = {PATH_SEPARATOR, '\0'};

    for (int i = 0; i < count; ++i) {
        const char *component = va_arg(args, const char*);
        if (!component) continue;

        size_t componentLen = strlen(component);
        if (componentLen == 0) continue;

        // Calculate required length
        // Need space for component, potentially a separator, and null terminator
        size_t needed = componentLen + (currentLen > 0 ? 1 : 0) + 1; 

        if (currentLen + needed > size) {
            va_end(args);
            setLastError(HYPERION_IO_NO_MEMORY);
            // Consider adding a HYPERION_IO_BUFFER_TOO_SMALL error code
            return HYPERION_IO_NO_MEMORY; 
        }

        // Add separator if not the first component and buffer not empty
        if (currentLen > 0) {
            // Avoid double separators if component starts with one or buffer ends with one
            if (buffer[currentLen - 1] != PATH_SEPARATOR && component[0] != PATH_SEPARATOR) {
                 strcat(buffer, separator);
                 currentLen++;
            } else if (buffer[currentLen - 1] == PATH_SEPARATOR && component[0] == PATH_SEPARATOR) {
                 component++;
                 componentLen--;
            }
        }
        
        // Concatenate component, handling potential leading separator skip
        strcat(buffer, component);
        currentLen += componentLen;
    }

    va_end(args);
    setLastError(HYPERION_IO_SUCCESS);
    return HYPERION_IO_SUCCESS;
}


const char* hyperionGetFileName(const char *path) {
    if (!path) return NULL;
    const char *lastSep = strrchr(path, PATH_SEPARATOR);
#ifdef _WIN32
    // Also check for forward slash on Windows
    const char *lastForwardSep = strrchr(path, '/');
    if (lastForwardSep > lastSep) {
        lastSep = lastForwardSep;
    }
#endif
    return (lastSep == NULL) ? path : lastSep + 1;
}

const char* hyperionGetFileExt(const char *path) {
     if (!path) return NULL;
     const char *fileName = hyperionGetFileName(path);
     const char *lastDot = strrchr(fileName, '.');
     // Ensure dot is not the first character of the filename
     return (lastDot == NULL || lastDot == fileName) ? NULL : lastDot + 1;
}

int hyperionGetDirName(const char *path, char *buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }

    const char *fileName = hyperionGetFileName(path);
    if (fileName == path) {
        // Return "." for current directory? Or empty string? Let's go with "."
        if (size < 2) {
             setLastError(HYPERION_IO_NO_MEMORY);
             return HYPERION_IO_NO_MEMORY;
        }
        strcpy(buffer, ".");
    } else {
        size_t dirLen = (size_t)(fileName - path);
        if (dirLen == 0) {
             if (size < 2) {
                 setLastError(HYPERION_IO_NO_MEMORY);
                 return HYPERION_IO_NO_MEMORY;
             }
             buffer[0] = PATH_SEPARATOR;
             buffer[1] = '\0';
        } else {
             // Remove trailing separator if present and it's not the root dir separator
             if (dirLen > 1 && path[dirLen - 1] == PATH_SEPARATOR) {
                 dirLen--;
             }
             if (dirLen + 1 > size) {
                 setLastError(HYPERION_IO_NO_MEMORY);
                 return HYPERION_IO_NO_MEMORY;
             }
             strncpy(buffer, path, dirLen);
             buffer[dirLen] = '\0';
        }
    }
    setLastError(HYPERION_IO_SUCCESS);
    return HYPERION_IO_SUCCESS;
}

int hyperionGetAbsPath(const char *path, char *buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        setLastError(HYPERION_IO_INVALID);
        return HYPERION_IO_INVALID;
    }

#ifdef _WIN32
    // Use Windows API GetFullPathName
    DWORD result = GetFullPathName(path, (DWORD)size, buffer, NULL);
    if (result == 0) {
        // Error occurred
        setLastErrorFromErrno();
        return g_lastIOError;
    } else if (result > size) {
        // Buffer too small
        setLastError(HYPERION_IO_NO_MEMORY);
        return HYPERION_IO_NO_MEMORY;
    } else {
        setLastError(HYPERION_IO_SUCCESS);
        return HYPERION_IO_SUCCESS;
    }
#else // POSIX
    // Use POSIX realpath
    char *real_path = realpath(path, buffer);
    if (real_path == NULL) {
        // If buffer is NULL, realpath allocates memory. If buffer is provided,
        // it uses the buffer. Error could be ENOENT, EACCES, ENAMETOOLONG, etc.
        // or ENOMEM if buffer is NULL and malloc fails.
        // If buffer is provided and too small, it returns NULL and sets errno to ERANGE.
        if (errno == ERANGE && buffer != NULL) {
             setLastError(HYPERION_IO_NO_MEMORY);
        } else {
             setLastErrorFromErrno();
        }
        return g_lastIOError;
    }
    // If realpath succeeded and used our buffer, real_path == buffer.
    // If realpath allocated memory (buffer was NULL), we'd need to copy and free.
    // Since we provide the buffer, we assume success means buffer is filled.
    setLastError(HYPERION_IO_SUCCESS);
    return HYPERION_IO_SUCCESS;
#endif
}