/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_FILESYSTEM_H
#define CAIS_FILESYSTEM_H

#include <stddef.h>
#include <cais/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * Types, Constants & Enumerations
 * =============================================================================
 */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
typedef unsigned long long cais_fsize_t;
#elif defined(_MSC_VER)
typedef unsigned __int64 cais_fsize_t;
#elif defined(__GNUC__) || defined(__clang__)
__extension__ typedef unsigned long long cais_fsize_t;
#else
typedef unsigned long cais_fsize_t;
#endif

#ifndef CAIS_PATH_MAX
#define CAIS_PATH_MAX 1024
#endif

typedef enum {
    CAIS_FS_TYPE_UNKNOWN   = 0,
    CAIS_FS_TYPE_FILE      = 1,
    CAIS_FS_TYPE_DIR       = 2,
    CAIS_FS_TYPE_SYMLINK   = 3,
    CAIS_FS_TYPE_DEVICE    = 4,
    CAIS_FS_TYPE_FIFO      = 5,
    CAIS_FS_TYPE_SOCKET    = 6
} cais_fs_type_t;

/**
 * @brief File or directory metadata status structure.
 */
typedef struct cais_fs_stat {
    cais_fs_type_t type;         /**< Node kind (regular file, directory, symlink, etc.) */
    cais_fsize_t   size;         /**< File size in bytes (64-bit on supported platforms) */
    long           mtime;        /**< Last modification timestamp (epoch seconds) */
    long           atime;        /**< Last access timestamp (epoch seconds) */
    long           ctime;        /**< Status change or file creation timestamp (epoch seconds) */
    int            mode;         /**< Permission bits (POSIX octal mode flags, e.g. 0644) */
    cais_bool_t    is_readonly;  /**< CAIS_TRUE if read-only attribute is set, CAIS_FALSE otherwise */
} cais_fs_stat_t;

/**
 * @brief Directory entry structure yielded during directory traversal.
 */
typedef struct cais_fs_dirent {
    char           name[256];    /**< Null-terminated entry filename (basename) */
    cais_fs_type_t type;         /**< Node kind of entry (file, directory, symlink, etc.) */
} cais_fs_dirent_t;

typedef struct cais_fs_dir cais_fs_dir_t;
typedef struct cais_file cais_file_t;

#define CAIS_FS_SEEK_SET 0
#define CAIS_FS_SEEK_CUR 1
#define CAIS_FS_SEEK_END 2

/*
 * =============================================================================
 * 1. Path Algebra & Pure Normalization (No Syscalls / Zero Disk I/O)
 * =============================================================================
 */

/*
 * Joins two path components using the native separator.
 * Returns CAIS_OK on success, or CAIS_ERROR if out_cap is insufficient.
 */
cais_status_t cais_fs_path_join(const char *p1, const char *p2, char *out, size_t out_cap);

/*
 * Normalizes a path in-place or into out:
 * - Resolves '.' and '..' components
 * - Collapses repeated separators
 * Returns CAIS_OK on success, or CAIS_ERROR if out_cap is insufficient.
 */
cais_status_t cais_fs_path_normalize(const char *path, char *out, size_t out_cap);

/*
 * Extracts the base name (final component) of path.
 * Returns CAIS_OK on success, or CAIS_ERROR if out_cap is insufficient.
 */
cais_status_t cais_fs_path_basename(const char *path, char *out, size_t out_cap);

/*
 * Extracts the directory name (parent component) of path.
 * Returns CAIS_OK on success, or CAIS_ERROR if out_cap is insufficient.
 */
cais_status_t cais_fs_path_dirname(const char *path, char *out, size_t out_cap);

/*
 * Extracts the file extension (including the leading dot, e.g. ".txt") of path.
 * If no extension exists, writes an empty string.
 * Returns CAIS_OK on success, or CAIS_ERROR if out_cap is insufficient.
 */
cais_status_t cais_fs_path_ext(const char *path, char *out, size_t out_cap);

/*
 * Checks whether the given path is absolute.
 * Returns CAIS_TRUE if absolute, CAIS_FALSE if relative or invalid.
 */
cais_bool_t cais_fs_path_is_absolute(const char *path);

/*
 * Returns the primary path separator character for the current platform.
 */
char cais_fs_path_separator(void);

/*
 * =============================================================================
 * 2. File & Directory Inspection (Metadata & Stat)
 * =============================================================================
 */

/*
 * Checks whether a file, device node, or directory exists at path.
 * Returns CAIS_TRUE if exists, CAIS_FALSE otherwise.
 */
cais_bool_t cais_fs_exists(const char *path);

/*
 * Retrieves metadata for the filesystem entry at path, following symlinks.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure (writing message to err_buf).
 */
cais_status_t cais_fs_stat(const char *path, cais_fs_stat_t *st, char *err_buf, size_t err_cap);

/*
 * Retrieves metadata for the filesystem entry at path without following symlinks.
 * On platforms without symlink support, behaves identically to cais_fs_stat.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure (writing message to err_buf).
 */
cais_status_t cais_fs_lstat(const char *path, cais_fs_stat_t *st, char *err_buf, size_t err_cap);

/*
 * Predicates: fast boolean checks.
 * Return CAIS_TRUE if condition holds, CAIS_FALSE if false or on error.
 */
cais_bool_t cais_fs_is_file(const char *path);
cais_bool_t cais_fs_is_dir(const char *path);
cais_bool_t cais_fs_is_symlink(const char *path);

/*
 * Retrieves file size directly.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_file_size(const char *path, cais_fsize_t *out_size);

/*
 * =============================================================================
 * 3. File System Operations & Mutation
 * =============================================================================
 */

/*
 * Creates a single directory at path with the given permission mode (e.g. 0755).
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_mkdir(const char *path, int mode, char *err_buf, size_t err_cap);

/*
 * Recursively creates parent directories as needed (equivalent to mkdir -p).
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_mkdir_p(const char *path, int mode, char *err_buf, size_t err_cap);

/*
 * Removes an empty directory at path.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_rmdir(const char *path, char *err_buf, size_t err_cap);

/*
 * Unlinks / deletes a file or symbolic link at path.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_remove(const char *path, char *err_buf, size_t err_cap);

/*
 * Recursively deletes a file or directory tree (equivalent to rm -rf).
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_remove_all(const char *path, char *err_buf, size_t err_cap);

/*
 * Renames or moves a file or directory from oldpath to newpath.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_rename(const char *oldpath, const char *newpath, char *err_buf, size_t err_cap);

/*
 * Copies a regular file from src to dst.
 * If overwrite is CAIS_FALSE and dst already exists, fails with CAIS_ERROR.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_copy_file(const char *src, const char *dst, cais_bool_t overwrite, char *err_buf, size_t err_cap);

/*
 * Changes the permission mode of a file or directory.
 * Returns CAIS_OK on success, or CAIS_ERROR / CAIS_UNSUPPORTED on failure.
 */
cais_status_t cais_fs_chmod(const char *path, int mode, char *err_buf, size_t err_cap);

/*
 * =============================================================================
 * 4. Directory Iteration (Streaming Iterator)
 * =============================================================================
 */

/*
 * Opens a directory for streaming iteration.
 * Returns an opaque cais_fs_dir_t handle, or NULL on failure.
 */
cais_fs_dir_t *cais_fs_dir_open(const char *path, char *err_buf, size_t err_cap);

/*
 * Reads the next directory entry.
 * Skips "." and ".." automatically.
 * Returns:
 *   CAIS_OK   : ent populated with the next entry name and type.
 *   CAIS_EOF  : no more entries in directory.
 *   CAIS_ERROR: iteration failed.
 */
cais_status_t cais_fs_dir_next(cais_fs_dir_t *dir, cais_fs_dirent_t *ent);

/*
 * Closes a directory handle and releases resources.
 */
void cais_fs_dir_close(cais_fs_dir_t *dir);

/*
 * =============================================================================
 * 5. File Content Operations (Slurp / Spit & Streaming Handles)
 * =============================================================================
 */

/*
 * Reads the entire contents of a file into a newly allocated buffer.
 * Caller is responsible for freeing *out_buf with free().
 * Null-terminates *out_buf (*out_buf[*out_len] == '\0') for text convenience.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_read_file(const char *path, void **out_buf, size_t *out_len, char *err_buf, size_t err_cap);

/*
 * Writes data into path, overwriting any existing contents.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_write_file(const char *path, const void *buf, size_t len, char *err_buf, size_t err_cap);

/*
 * Appends data to path, creating the file if it does not exist.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_append_file(const char *path, const void *buf, size_t len, char *err_buf, size_t err_cap);

/*
 * Opens a file stream with mode string ("r", "w", "a", "r+", "w+", "a+").
 * Returns an allocated cais_file_t handle, or NULL on failure.
 */
cais_file_t *cais_file_open(const char *path, const char *mode, char *err_buf, size_t err_cap);

/*
 * Reads up to count bytes from file into buf.
 * Populates *bytes_read with the actual number of bytes read.
 * Returns CAIS_OK on success, or CAIS_EOF / CAIS_ERROR.
 */
cais_status_t cais_file_read(cais_file_t *f, void *buf, size_t count, size_t *bytes_read);

/*
 * Writes up to count bytes from buf into file.
 * Populates *bytes_written with the actual number of bytes written.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_file_write(cais_file_t *f, const void *buf, size_t count, size_t *bytes_written);

/*
 * Sets the file position indicator.
 * whence: CAIS_FS_SEEK_SET, CAIS_FS_SEEK_CUR, CAIS_FS_SEEK_END.
 * If new_pos is non-NULL, writes the resulting file offset.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_file_seek(cais_file_t *f, long offset, int whence, long *new_pos);

/*
 * Obtains the current file position indicator.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_file_tell(cais_file_t *f, long *pos);

/*
 * Flushes buffered output to disk.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_file_flush(cais_file_t *f);

/*
 * Closes the file stream and releases allocated memory.
 */
void cais_file_close(cais_file_t *f);

/*
 * =============================================================================
 * 6. Environment & Special Directories
 * =============================================================================
 */

/*
 * Gets the current working directory of the process.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_getcwd(char *buf, size_t cap, char *err_buf, size_t err_cap);

/*
 * Changes the current working directory of the process.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_setcwd(const char *path, char *err_buf, size_t err_cap);

/*
 * Obtains the path to the operating system's temporary directory.
 * Returns CAIS_OK on success, or CAIS_ERROR on failure.
 */
cais_status_t cais_fs_temp_dir(char *buf, size_t cap, char *err_buf, size_t err_cap);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_FILESYSTEM_H */
