# Filesystem & Path I/O (`cais.filesystem` / `fs`)

The `filesystem` subsystem (accessible via both `cais.filesystem` and the convenient shorthand `cais.fs`) provides a complete, portable abstraction layer for file and directory manipulation.

It spans zero-allocation path algebra, streaming directory traversal, unified whole-file operations, structured file streaming with seek/tell indicators, permission manipulation, and metadata introspection across modern POSIX systems, Windows NT, MS-DOS FAT, AmigaOS, Atari TOS, and Plan 9.

---

## ⚡ 1. Architectural Principles & Guarantees

### 1.1 Pure Path Algebra (Zero Syscall / Zero Disk I/O)
Path manipulation functions (`cais_fs_path_join`, `cais_fs_path_normalize`, `cais_fs_path_basename`, `cais_fs_path_dirname`, `cais_fs_path_ext`, `cais_fs_path_is_absolute`) operate **strictly in memory**:
* They perform **zero system calls** and never access physical media.
* They guarantee zero heap allocations when using caller-supplied stack buffers.
* They resolve relative dot components (`.` and `..`), collapse repeated separators (`a///b` -> `a/b`), and handle root prefixes across Unix (`/`), Windows (`C:\`, `\\server\share`), and Amiga assigns (`SYS:`, `T:`).

### 1.2 Dual Module Identity in Lua
In Lua, `require("cais.fs")` and `require("cais.filesystem")` resolve to the exact same table instance. Developers can choose between terse idiomatic code (`fs.read()`, `fs.join()`) or formal enterprise code (`filesystem.read_file()`, `filesystem.path.join()`).

### 1.3 64-Bit File Size Guarantee (`cais_fsize_t`)
File sizes in `cais_fs_stat_t` are defined via `cais_fsize_t` (`unsigned long long` on C99/modern compilers, `unsigned __int64` on MSVC, or `unsigned long` on 16/32-bit legacy compilers). This ensures transparent support for large files (> 2 GB and > 4 GB) without 32-bit integer overflows.

### 1.4 Safe Directory Streaming Iteration
Unlike standard POSIX `readdir()` which returns raw `.` (current directory) and `..` (parent directory) entries, `cais_fs_dir_next()` automatically skips these pseudo-directories. This prevents accidental infinite recursion in directory traversal algorithms.

---

## ⚙️ 2. Constants & Enumerations

### 2.1 Filesystem Node Types (`cais_fs_type_t`)

Returned by `cais_fs_stat()` and directory iterators:

| C Constant             | Lua Field (`fs.type`) | Value | Description                                            |
| :--------------------- | :-------------------- | :---: | :----------------------------------------------------- |
| `CAIS_FS_TYPE_UNKNOWN` | `UNKNOWN`             |  `0`  | Unidentifiable node kind or inaccessible entry         |
| `CAIS_FS_TYPE_FILE`    | `FILE`                |  `1`  | Regular data file                                      |
| `CAIS_FS_TYPE_DIR`     | `DIR`                 |  `2`  | Directory / folder container                           |
| `CAIS_FS_TYPE_SYMLINK` | `SYMLINK`             |  `3`  | Symbolic link (reparse point on Windows)               |
| `CAIS_FS_TYPE_DEVICE`  | `DEVICE`              |  `4`  | Character device (`/dev/null`, `COM1`) or block device |
| `CAIS_FS_TYPE_FIFO`    | `FIFO`                |  `5`  | Named pipe                                             |
| `CAIS_FS_TYPE_SOCKET`  | `SOCKET`              |  `6`  | UNIX domain socket                                     |

---

### 2.2 Seek Origins (`cais_file_seek`)

| C Constant         | Lua Value | Description                                              |
| :----------------- | :-------: | :------------------------------------------------------- |
| `CAIS_FS_SEEK_SET` |  `"set"`  | Seek relative to the beginning of the file (offset >= 0) |
| `CAIS_FS_SEEK_CUR` |  `"cur"`  | Seek relative to the current file pointer position       |
| `CAIS_FS_SEEK_END` |  `"end"`  | Seek relative to the physical end of the file            |

---

### 2.3 Status Codes

| C Constant         | Value | Description                                                         |
| :----------------- | :---: | :------------------------------------------------------------------ |
| `CAIS_OK`          |  `0`  | Operation succeeded                                                 |
| `CAIS_ERROR`       | `-1`  | General I/O or filesystem error (details written to `err_buf`)      |
| `CAIS_EOF`         | `-2`  | End-of-file reached on file or directory stream                     |
| `CAIS_UNSUPPORTED` | `-4`  | Operation not supported by host filesystem (e.g. symlinks on FAT16) |

---

## 🏛️ 3. ANSI C89 Function Reference

All functions are declared in [`include/cais/filesystem.h`](file:///home/gustavo/Projetos/cais/include/cais/filesystem.h).

### 3.1 Pure Path Algebra (Zero Disk I/O)

```c
/* Joins two paths using platform separator ('/' or '\\') */
cais_status_t cais_fs_path_join(const char *p1, const char *p2, char *out, size_t out_cap);

/* Collapses '.', '..', and redundant separators */
cais_status_t cais_fs_path_normalize(const char *path, char *out, size_t out_cap);

/* Extracts filename (e.g. "dir/file.txt" -> "file.txt") */
cais_status_t cais_fs_path_basename(const char *path, char *out, size_t out_cap);

/* Extracts parent directory (e.g. "dir/file.txt" -> "dir") */
cais_status_t cais_fs_path_dirname(const char *path, char *out, size_t out_cap);

/* Extracts extension with dot (e.g. "archive.tar.gz" -> ".gz") */
cais_status_t cais_fs_path_ext(const char *path, char *out, size_t out_cap);

/* Checks if path starts with root '/' or drive letter 'C:\' */
cais_bool_t cais_fs_path_is_absolute(const char *path);

/* Returns '/' on POSIX/Amiga/Atari, or '\\' on Windows/DOS */
char cais_fs_path_separator(void);
```

---

### 3.2 Metadata & Inspection (`stat`)

```c
/* Fast existence checks */
cais_bool_t cais_fs_exists(const char *path);
cais_bool_t cais_fs_is_file(const char *path);
cais_bool_t cais_fs_is_dir(const char *path);
cais_bool_t cais_fs_is_symlink(const char *path);
cais_status_t cais_fs_file_size(const char *path, cais_fsize_t *out_size);

/* Inspect full metadata (follows symlinks) */
cais_status_t cais_fs_stat(const char *path, cais_fs_stat_t *st, char *err_buf, size_t err_cap);

/* Inspect metadata of symlink itself (does not follow symlinks) */
cais_status_t cais_fs_lstat(const char *path, cais_fs_stat_t *st, char *err_buf, size_t err_cap);
```

#### `cais_fs_stat_t` Structure:
```c
typedef struct cais_fs_stat {
    cais_fs_type_t type;         /* Node kind: FILE, DIR, SYMLINK, etc. */
    cais_fsize_t   size;         /* Size in bytes (64-bit safe) */
    long           mtime;        /* Last modification time (epoch seconds) */
    long           atime;        /* Last access time (epoch seconds) */
    long           ctime;        /* Status change / creation time */
    int            mode;         /* Permission bits (POSIX octal mode flags, e.g. 0644) */
    cais_bool_t    is_readonly;  /* CAIS_TRUE if write-protected */
} cais_fs_stat_t;
```

---

### 3.3 Directory Iteration (Streaming Iterator)

```c
cais_fs_dir_t *cais_fs_dir_open(const char *path, char *err_buf, size_t err_cap);
cais_status_t  cais_fs_dir_next(cais_fs_dir_t *dir, cais_fs_dirent_t *ent);
void           cais_fs_dir_close(cais_fs_dir_t *dir);
```
* **`cais_fs_dirent_t` Fields:**
  * `char name[256]`: Entry filename (null-terminated basename).
  * `cais_fs_type_t type`: Entry node kind (`CAIS_FS_TYPE_FILE`, `CAIS_FS_TYPE_DIR`, etc.).
* **Iteration Cycle:**
  Calls to `cais_fs_dir_next()` return `CAIS_OK` while entries exist, `CAIS_EOF` when exhausted, or `CAIS_ERROR` on fault. Automatically skips `.` and `..`.

---

### 3.4 File System Operations & Mutation

```c
/* Create directories */
cais_status_t cais_fs_mkdir(const char *path, int mode, char *err_buf, size_t err_cap);
cais_status_t cais_fs_mkdir_p(const char *path, int mode, char *err_buf, size_t err_cap);

/* Delete entries */
cais_status_t cais_fs_rmdir(const char *path, char *err_buf, size_t err_cap);
cais_status_t cais_fs_remove(const char *path, char *err_buf, size_t err_cap);
cais_status_t cais_fs_remove_all(const char *path, char *err_buf, size_t err_cap);

/* Rename / Move / Copy */
cais_status_t cais_fs_rename(const char *oldpath, const char *newpath, char *err_buf, size_t err_cap);
cais_status_t cais_fs_copy_file(const char *src, const char *dst, cais_bool_t overwrite, char *err_buf, size_t err_cap);
cais_status_t cais_fs_chmod(const char *path, int mode, char *err_buf, size_t err_cap);
```

---

### 3.5 Whole-File Content I/O (Slurp & Spit)

```c
/* Reads entire file into newly allocated buffer (caller frees with free()) */
cais_status_t cais_fs_read_file(const char *path, void **out_buf, size_t *out_len, char *err_buf, size_t err_cap);

/* Writes data to file, truncating existing content */
cais_status_t cais_fs_write_file(const char *path, const void *buf, size_t len, char *err_buf, size_t err_cap);

/* Appends data to file, creating it if absent */
cais_status_t cais_fs_append_file(const char *path, const void *buf, size_t len, char *err_buf, size_t err_cap);
```
> [!NOTE]
> `cais_fs_read_file()` appends a trailing null byte (`\0`) past `*out_len` without including it in the byte count, making it immediately usable as a C string.

---

### 3.6 Streaming File Handles (`cais_file_t`)

```c
cais_file_t  *cais_file_open(const char *path, const char *mode, char *err_buf, size_t err_cap);
cais_status_t cais_file_read(cais_file_t *f, void *buf, size_t count, size_t *bytes_read);
cais_status_t cais_file_write(cais_file_t *f, const void *buf, size_t count, size_t *bytes_written);
cais_status_t cais_file_seek(cais_file_t *f, long offset, int whence, long *new_pos);
cais_status_t cais_file_tell(cais_file_t *f, long *pos);
cais_status_t cais_file_flush(cais_file_t *f);
void          cais_file_close(cais_file_t *f);
```

---

### 3.7 Environment & Working Directory

```c
cais_status_t cais_fs_getcwd(char *buf, size_t cap, char *err_buf, size_t err_cap);
cais_status_t cais_fs_setcwd(const char *path, char *err_buf, size_t err_cap);
cais_status_t cais_fs_temp_dir(char *buf, size_t cap, char *err_buf, size_t err_cap);
```

---

## 💻 4. Practical C89 Programming Examples

### Example 1: Recursive Directory Tree Scanner & Size Aggregator
Walks a directory tree recursively, reporting all files and accumulating total byte consumption:

```c
#include <cais/filesystem.h>
#include <stdio.h>
#include <string.h>

cais_fsize_t walk_directory_tree(const char *root_path) {
    char err[128];
    char child_path[CAIS_PATH_MAX];
    cais_fsize_t total_bytes = 0;
    cais_fs_dirent_t ent;
    cais_fs_dir_t *dir = cais_fs_dir_open(root_path, err, sizeof(err));

    if (!dir) {
        fprintf(stderr, "Cannot open directory '%s': %s\n", root_path, err);
        return 0;
    }

    while (cais_fs_dir_next(dir, &ent) == CAIS_OK) {
        if (cais_fs_path_join(root_path, ent.name, child_path, sizeof(child_path)) != CAIS_OK) {
            continue;
        }

        if (ent.type == CAIS_FS_TYPE_DIR) {
            /* Recurse into child directory */
            total_bytes += walk_directory_tree(child_path);
        } else if (ent.type == CAIS_FS_TYPE_FILE) {
            cais_fs_stat_t st;
            if (cais_fs_stat(child_path, &st, NULL, 0) == CAIS_OK) {
                total_bytes += st.size;
                printf("FILE: %-40s | %8lu bytes\n", child_path, (unsigned long)st.size);
            }
        }
    }

    cais_fs_dir_close(dir);
    return total_bytes;
}

int main(void) {
    cais_fsize_t total = walk_directory_tree("src");
    printf("Total source tree size: %lu bytes\n", (unsigned long)total);
    return 0;
}
```

---

### Example 2: Structured Binary Record Logger with Seek & Tell
Demonstrates streaming file handles with byte offsets:

```c
#include <cais/filesystem.h>
#include <stdio.h>

typedef struct {
    unsigned long timestamp;
    int event_id;
    char message[32];
} log_record_t;

int main(void) {
    char err[128];
    long offset = 0;
    size_t written = 0;
    log_record_t r;
    cais_file_t *f = cais_file_open("audit.log", "w+", err, sizeof(err));

    if (!f) {
        fprintf(stderr, "Failed to open audit log: %s\n", err);
        return 1;
    }

    /* Record 1 */
    r.timestamp = 1772755200UL;
    r.event_id = 101;
    strncpy(r.message, "System startup", sizeof(r.message));
    cais_file_write(f, &r, sizeof(r), &written);

    /* Record 2 */
    r.timestamp = 1772755205UL;
    r.event_id = 102;
    strncpy(r.message, "Interface UP", sizeof(r.message));
    cais_file_write(f, &r, sizeof(r), &written);

    /* Query current position */
    cais_file_tell(f, &offset);
    printf("Wrote 2 records. Current file offset: %ld bytes\n", offset);

    /* Rewind to record 1 and read it back */
    cais_file_seek(f, 0, CAIS_FS_SEEK_SET, NULL);
    cais_file_read(f, &r, sizeof(r), &written);
    printf("Read back record 1: ID=%d, Msg='%s'\n", r.event_id, r.message);

    cais_file_flush(f);
    cais_file_close(f);
    return 0;
}
```

---

## 🌙 5. Practical Lua Programming Examples (Lua >= 5.4)

The Lua module is loaded via `require("cais.fs")` or `require("cais.filesystem")`.

### 5.1 Lua API Quick Reference

| Function / Method       | Signature                      | Description                                                              |
| :---------------------- | :----------------------------- | :----------------------------------------------------------------------- |
| `fs.exists(path)`       | `fs.exists(path)`              | Returns boolean `true` if node exists, `false` otherwise.                |
| `fs.is_file(path)`      | `fs.is_file(path)`             | Returns `true` if path is a regular file.                                |
| `fs.is_dir(path)`       | `fs.is_dir(path)`              | Returns `true` if path is a directory.                                   |
| `fs.stat(path)`         | `fs.stat(path)`                | Returns table (`type`, `size`, `mtime`, `atime`, `mode`, `is_readonly`). |
| `fs.read(path)`         | `fs.read(path)`                | Reads entire file into string. Returns `str` or `nil, err`.              |
| `fs.write(path, str)`   | `fs.write(path, data)`         | Overwrites file with data. Returns `true` or `nil, err`.                 |
| `fs.append(path, str)`  | `fs.append(path, data)`        | Appends data to file. Returns `true` or `nil, err`.                      |
| `fs.mkdir(path [, r])`  | `fs.mkdir(path, [recursive])`  | Creates directory (recursive if second argument is `true`).              |
| `fs.remove(path [, r])` | `fs.remove(path, [recursive])` | Deletes file or directory tree.                                          |
| `fs.copy(src, dst)`     | `fs.copy(src, dst)`            | Copies file to destination.                                              |
| `fs.rename(old, new)`   | `fs.rename(old, new)`          | Moves or renames entry.                                                  |
| `fs.dir(path)`          | `for entry in fs.dir(path)`    | Streaming iterator yielding `{ name = "...", type = "file"               | "dir" }`. |
| `fs.list(path)`         | `fs.list(path)`                | Returns array table of entry names in directory.                         |
| `fs.open(path, mode)`   | `fs.open(path, mode)`          | Opens stream handle (`f:read()`, `f:write()`, `f:seek()`, `f:close()`).  |
| `fs.cwd()`              | `fs.cwd()`                     | Returns current working directory path string.                           |
| `fs.temp_dir()`         | `fs.temp_dir()`                | Returns OS temporary directory path string.                              |

---

### Example 1: Recursive File Backup & Compression Staging Script

```lua
local fs = require("cais.fs")

local function backup_tree(src_dir, backup_dir)
    if not fs.exists(src_dir) then
        error("Source directory does not exist: " .. src_dir)
    end

    -- Create target backup folder recursively
    fs.mkdir(backup_dir, true)

    for entry in fs.dir(src_dir) do
        local src_path = fs.path.join(src_dir, entry.name)
        local dst_path = fs.path.join(backup_dir, entry.name)

        if entry.type == "dir" then
            -- Recurse into subfolder
            backup_tree(src_path, dst_path)
        elseif entry.type == "file" then
            local info = fs.stat(src_path)
            print(string.format("Backing up: %-30s (%d bytes)", entry.name, info.size))
            fs.copy(src_path, dst_path)
        end
    end
end

backup_tree("docs", "build/backup/docs")
print("Backup completed successfully to " .. fs.path.normalize("build/backup/docs"))
```

---

### Example 2: Stream Processing with Lua 5.4 `<close>` To-Be-Closed Resource Handling

```lua
local fs = require("cais.fs")

local function count_lines_in_file(path)
    -- Lua 5.4 <close> variable automatically calls f:close() on exit or on error
    local f <close>, err = fs.open(path, "r")
    if not f then
        return nil, err
    end

    local line_count = 0
    local buffer = f:read(4096)
    while buffer and #buffer > 0 do
        for _ in buffer:gmatch("\n") do
            line_count = line_count + 1
        end
        buffer = f:read(4096)
    end

    return line_count
end

local lines, err = count_lines_in_file("README.md")
if lines then
    print("README.md contains " .. lines .. " lines.")
else
    print("Error reading file: " .. tostring(err))
end
```