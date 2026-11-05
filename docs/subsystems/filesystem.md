# Filesystem & Path I/O (`cais.filesystem` / `fs`)

The `filesystem` subsystem (available as both `cais.filesystem` and `cais.fs`) provides zero-allocation path algebra, recursive file and directory manipulation, metadata inspection, and streaming file I/O.

---

## 1. Constants & Enumerations

### Filesystem Node Type (`cais_fs_type_t`)

| C Constant | Lua Field (`cais.fs.type`) | Integer Value | Description |
| :--- | :--- | :---: | :--- |
| `CAIS_FS_TYPE_UNKNOWN` | `UNKNOWN` | `0` | Unidentifiable node type |
| `CAIS_FS_TYPE_FILE` | `FILE` | `1` | Regular file |
| `CAIS_FS_TYPE_DIR` | `DIR` | `2` | Directory |
| `CAIS_FS_TYPE_SYMLINK` | `SYMLINK` | `3` | Symbolic link |
| `CAIS_FS_TYPE_DEVICE` | `DEVICE` | `4` | Character or block special device |
| `CAIS_FS_TYPE_FIFO` | `FIFO` | `5` | Named pipe (FIFO) |
| `CAIS_FS_TYPE_SOCKET` | `SOCKET` | `6` | UNIX domain socket |

### Seek Origins

| C Constant | Lua Value | Description |
| :--- | :--- | :--- |
| `CAIS_FS_SEEK_SET` | `"set"` | Beginning of file |
| `CAIS_FS_SEEK_CUR` | `"cur"` | Current file offset position |
| `CAIS_FS_SEEK_END` | `"end"` | End of file |

---

## 2. C89 Function Reference

### 2.1 Pure Path Algebra (Zero Disk I/O)

These functions do not execute syscalls or touch the disk:

```c
/* Joins two path components with platform separator */
cais_status_t cais_fs_path_join(const char *p1, const char *p2, char *out, size_t out_cap);

/* Collapses '.', '..', and duplicate slashes */
cais_status_t cais_fs_path_normalize(const char *path, char *out, size_t out_cap);

/* Extracts the final component (e.g. "file.txt") */
cais_status_t cais_fs_path_basename(const char *path, char *out, size_t out_cap);

/* Extracts the parent directory component (e.g. "/usr/local") */
cais_status_t cais_fs_path_dirname(const char *path, char *out, size_t out_cap);

/* Extracts extension including dot (e.g. ".tar.gz" -> ".gz") */
cais_status_t cais_fs_path_ext(const char *path, char *out, size_t out_cap);

/* Returns CAIS_TRUE if path begins with root or drive letter */
cais_bool_t cais_fs_path_is_absolute(const char *path);

/* Returns '/' on POSIX/Amiga/Atari, or '\\' on Windows/DOS */
char cais_fs_path_separator(void);
```

---

### 2.2 File & Directory Inspection

```c
/* Fast existence predicate */
cais_bool_t cais_fs_exists(const char *path);
cais_bool_t cais_fs_is_file(const char *path);
cais_bool_t cais_fs_is_dir(const char *path);
cais_bool_t cais_fs_is_symlink(const char *path);

/* Inspect metadata (size, timestamps, permissions) */
cais_status_t cais_fs_stat(const char *path, cais_fs_stat_t *st, char *err_buf, size_t err_cap);
cais_status_t cais_fs_lstat(const char *path, cais_fs_stat_t *st, char *err_buf, size_t err_cap);
```

#### `cais_fs_stat_t` Structure:
```c
typedef struct cais_fs_stat {
    cais_fs_type_t type;         /* Node type */
    cais_fsize_t   size;         /* Size in bytes (64-bit safe) */
    long           mtime;        /* Modification time (epoch seconds) */
    long           atime;        /* Access time (epoch seconds) */
    long           ctime;        /* Status change time */
    int            mode;         /* Permission bits (POSIX octal mode flags) */
    cais_bool_t    is_readonly;  /* CAIS_TRUE if write-protected */
} cais_fs_stat_t;
```

---

### 2.3 Whole File I/O & Manipulations

```c
/* Reads entire file into dynamically allocated null-terminated buffer (caller frees with free()) */
char *cais_fs_read_file(const char *path, size_t *out_size);

/* Writes or appends entire buffer to file */
cais_status_t cais_fs_write_file(const char *path, const char *data, size_t len);
cais_status_t cais_fs_append_file(const char *path, const char *data, size_t len);

/* File system operations */
cais_status_t cais_fs_remove(const char *path);
cais_status_t cais_fs_remove_r(const char *path);
cais_status_t cais_fs_rename(const char *old_path, const char *new_path);
cais_status_t cais_fs_copy_file(const char *src, const char *dst);
cais_status_t cais_fs_copy_r(const char *src, const char *dst);
cais_status_t cais_fs_mkdir(const char *path);
cais_status_t cais_fs_mkdir_p(const char *path);
```

---

## 3. Lua API Reference

```lua
local fs = require("cais.fs") -- or require("cais.filesystem")
```

### Path Helpers
* `fs.join(p1, p2)`: Joins two paths.
* `fs.normalize(path)`: Normalizes path.
* `fs.basename(path)`: Returns filename.
* `fs.dirname(path)`: Returns directory name.
* `fs.ext(path)`: Returns file extension.
* `fs.is_absolute(path)`: Returns boolean.
* `fs.separator`: Primary separator character (`"/"` or `"\\"`).

### File & Directory Queries
* `fs.exists(path)`: Returns `true` or `false`.
* `fs.is_file(path)` / `fs.is_dir(path)` / `fs.is_symlink(path)`: Boolean predicates.
* `fs.stat(path)`: Returns table with fields `type`, `size`, `mtime`, `atime`, `ctime`, `mode`, `is_readonly`.
* `fs.list_dir(path)`: Returns array table of entry names.

### High-Level Operations
* `fs.read_file(path)`: Returns file contents as string.
* `fs.write_file(path, data)`: Overwrites file with data.
* `fs.append_file(path, data)`: Appends data to file.
* `fs.mkdir(path)` / `fs.mkdir_p(path)`: Creates directory (or parent tree).
* `fs.remove(path)` / `fs.remove_r(path)`: Deletes file / directory tree.
* `fs.copy(src, dst)` / `fs.copy_r(src, dst)`: Copies file or directory tree.
