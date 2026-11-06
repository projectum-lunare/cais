# cais.filesystem / cais.fs

The `cais.filesystem` module (aliased as `cais.fs`) provides a complete, portable Virtual Filesystem (VFS) with streaming file I/O, atomic read/write helpers, directory traversal, and cross-platform path manipulation.

---

## Requiring the Module

```lua
local fs = require("cais.fs")
-- or via full name:
local fs = require("cais.filesystem")
```

---

## Path Manipulation (`fs.path`)

Cross-platform path decomposition and normalization:

| Function              | Signature                   | Description                                                        |
| :-------------------- | :-------------------------- | :----------------------------------------------------------------- |
| `fs.path.join`        | `(...) -> string`           | Concatenates path segments using the target OS separator           |
| `fs.path.dirname`     | `(path: string) -> string`  | Returns parent directory component                                 |
| `fs.path.basename`    | `(path: string) -> string`  | Returns trailing filename component                                |
| `fs.path.ext`         | `(path: string) -> string`  | Returns file extension including the leading dot (or empty string) |
| `fs.path.normalize`   | `(path: string) -> string`  | Resolves `.` and `..` relative segments and cleans slashes         |
| `fs.path.is_absolute` | `(path: string) -> boolean` | Checks whether the path is absolute                                |

---

## Atomic File Helpers

### `fs.read(path)`

Read the entire contents of a file into a Lua string.

```lua
local content, err = fs.read("config.json")
if not content then
    error("Failed to read: " .. err)
end
```

### `fs.write(path, data)`

Write a string to a file, truncating any existing content.

```lua
local ok, err = fs.write("output.txt", "Hello from Cais Lua!\n")
```

### `fs.append(path, data)`

Append a string to the end of a file.

```lua
local ok, err = fs.append("log.txt", "[INFO] Event processed\n")
```

---

## File System Inspection & Metadata

### `fs.exists(path)`

Check whether a file, directory, or symlink exists.

* **Returns:** `boolean`

### `fs.stat(path)`

Retrieve metadata for `path`, following symbolic links.

* **Returns:** `table` with fields:
  * `size`: File size in bytes (`integer`).
  * `mode`: Raw POSIX permission mode bits (`integer`).
  * `atime`, `mtime`, `ctime`: Epoch timestamps (`integer`).
  * `is_readonly`: `boolean`.
  * `is_file`, `is_dir`, `is_symlink`: `boolean`.

### `fs.lstat(path)`

Same as `fs.stat()`, but does not follow symlinks (retrieves symlink metadata directly).

### Predicate Functions

* `fs.is_file(path) -> boolean`
* `fs.is_dir(path) -> boolean`
* `fs.is_symlink(path) -> boolean`
* `fs.file_size(path) -> integer|nil, string?`

---

## Directory Operations

### `fs.list(path)`

Enumerate directory entries, returning a table of entry objects.

```lua
local entries, err = fs.list("/var/log")
for _, entry in ipairs(entries) do
    print(string.format("%s (%s)", entry.name, entry.type))
end
```

### `fs.dir(path)`

Returns an iterator function that yields the basename of each entry:

```lua
for filename in fs.dir("./src") do
    print(filename)
end
```

### `fs.mkdir(path, recursive)`

Create a new directory. When `recursive` is `true`, parent directories are created automatically.

```lua
fs.mkdir("build/temp/cache", true)
```

### `fs.rmdir(path)`

Remove an empty directory.

### `fs.remove(path, recursive)`

Delete a file or directory. If `recursive` is `true`, deletes subdirectories and their contents.

### `fs.rename(old_path, new_path)`

Rename or move a file or directory.

### `fs.copy(src, dst)`

Copy a file from `src` to `dst`.

---

## Environment & Working Directory

* `fs.cwd() -> string|nil, string?`: Get current working directory.
* `fs.chdir(path) -> boolean|nil, string?`: Change current working directory.
* `fs.temp_dir() -> string`: Retrieve system temporary folder path (e.g. `/tmp` or `C:\Users\...\Temp`).

---

## Streaming Handle (`FileHandle`)

### `fs.open(path, mode)`

Open a file handle for streaming access.

* **Modes:** `"r"`, `"w"`, `"a"`, `"r+"`, `"w+"`, `"a+"` (add `"b"` for binary on Windows/DOS).
* **Returns:** `FileHandle` object or `nil, err`.

```lua
local handle, err = fs.open("large_payload.bin", "rb")
if not handle then
    error(err)
end

-- Read in 64KB chunks
while true do
    local chunk = handle:read(65536)
    if not chunk or #chunk == 0 then break end
    process(chunk)
end

handle:close()
```

### `FileHandle` Methods

* `handle:read(count?) -> string|nil, string?`: Read up to `count` bytes (reads all if omitted).
* `handle:write(data) -> integer|nil, string?`: Write string buffer, returns bytes written.
* `handle:seek(whence, offset) -> integer|nil, string?`: Seek anchor (`"set"`, `"cur"`, `"end"`).
* `handle:tell() -> integer|nil, string?`: Get current byte offset.
* `handle:flush() -> boolean|nil, string?`: Flush buffers to disk.
* `handle:close() -> boolean|nil, string?`: Explicitly close stream.
