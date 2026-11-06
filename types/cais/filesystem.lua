---@meta cais.filesystem

---
---# cais.filesystem / cais.fs
---
---Portable Virtual Filesystem (VFS), streaming file I/O, directory traversal,
---and path manipulation substrate.
---
local fs = {}

---@class CaisFileStat
---@field size integer File size in bytes
---@field mode integer Raw POSIX mode bits
---@field atime integer Last access time epoch
---@field mtime integer Last modification time epoch
---@field ctime integer Creation / status change time epoch
---@field is_readonly boolean True if file cannot be written to
---@field is_file boolean True if entry is a regular file
---@field is_dir boolean True if entry is a directory
---@field is_symlink boolean True if entry is a symbolic link

---@class CaisDirEntry
---@field name string Basename of directory entry
---@field type "file"|"dir"|"symlink"|"other" Entry type classifier

---@class CaisFileHandle
local FileHandle = {}

---Read up to `count` bytes from open stream.
---@param count? integer Number of bytes to read (reads entire file if omitted)
---@return string|nil data, string? err
function FileHandle:read(count) end

---Write binary or text data to file stream.
---@param data string Data payload to write
---@return integer|nil bytes_written, string? err
function FileHandle:write(data) end

---Reposition stream offset.
---@param whence "set"|"cur"|"end" Seek anchor
---@param offset integer Byte offset from whence
---@return integer|nil new_offset, string? err
function FileHandle:seek(whence, offset) end

---Retrieve current stream byte offset.
---@return integer|nil offset, string? err
function FileHandle:tell() end

---Flush pending stream buffers to underlying storage.
---@return boolean|nil success, string? err
function FileHandle:flush() end

---Explicitly close file handle and release system descriptor.
---@return boolean|nil success, string? err
function FileHandle:close() end

fs.path = {}

---Join two or more path segments using host canonical separator.
---@param ... string Path segments to concatenate
---@return string joined_path
function fs.path.join(...) end

---Extract directory component of path.
---@param path string Source path
---@return string dirname
function fs.path.dirname(path) end

---Extract final filename component of path.
---@param path string Source path
---@return string basename
function fs.path.basename(path) end

---Extract filename extension including leading dot.
---@param path string Source path
---@return string extension
function fs.path.ext(path) end

---Normalize path resolving relative segments (`.`, `..`) and slashes.
---@param path string Source path
---@return string normalized_path
function fs.path.normalize(path) end

---Check if path is absolute according to host OS semantics.
---@param path string Path to check
---@return boolean is_abs
function fs.path.is_absolute(path) end

---Check whether a file or directory exists at path.
---@param path string Path to test
---@return boolean exists
function fs.exists(path) end

---Retrieve metadata status for path following symlinks.
---@param path string Target path
---@return CaisFileStat|nil stat, string? err
function fs.stat(path) end

---Retrieve metadata status without following symlinks.
---@param path string Target path
---@return CaisFileStat|nil stat, string? err
function fs.lstat(path) end

---Check if target path is a regular file.
---@param path string Path to test
---@return boolean is_file
function fs.is_file(path) end

---Check if target path is a directory.
---@param path string Path to test
---@return boolean is_dir
function fs.is_dir(path) end

---Check if target path is a symbolic link.
---@param path string Path to test
---@return boolean is_symlink
function fs.is_symlink(path) end

---Retrieve byte size of file.
---@param path string Target file
---@return integer|nil size, string? err
function fs.file_size(path) end

---Read entire file contents into memory in a single atomic call.
---@param path string File to read
---@return string|nil content, string? err
function fs.read(path) end

---Write string content to target file (truncates existing).
---@param path string Target path
---@param content string Data to write
---@return boolean|nil success, string? err
function fs.write(path, content) end

---Append string content to target file.
---@param path string Target path
---@param content string Data to append
---@return boolean|nil success, string? err
function fs.append(path, content) end

---Create a new directory (optionally recursive).
---@param path string Directory path
---@param recursive? boolean Create parent directories if true
---@return boolean|nil success, string? err
function fs.mkdir(path, recursive) end

---Remove an empty directory.
---@param path string Directory path
---@return boolean|nil success, string? err
function fs.rmdir(path) end

---Remove file or directory (optionally recursive).
---@param path string Target path
---@param recursive? boolean Recursively delete contents if directory
---@return boolean|nil success, string? err
function fs.remove(path, recursive) end

---Rename or move a filesystem node.
---@param old_path string Source path
---@param new_path string Destination path
---@return boolean|nil success, string? err
function fs.rename(old_path, new_path) end

---Copy a file to a new location.
---@param src string Source file
---@param dst string Destination file
---@return boolean|nil success, string? err
function fs.copy(src, dst) end

---Change file permission mode bits.
---@param path string Target path
---@param mode integer Octal permission mask (e.g. 0o755)
---@return boolean|nil success, string? err
function fs.chmod(path, mode) end

---Get current working directory.
---@return string|nil cwd, string? err
function fs.cwd() end

---Change current working directory.
---@param path string Directory to switch to
---@return boolean|nil success, string? err
function fs.chdir(path) end

---Retrieve host system temporary directory.
---@return string temp_dir
function fs.temp_dir() end

---List directory contents returning an array of entry tables.
---@param path string Directory path
---@return CaisDirEntry[]|nil entries, string? err
function fs.list(path) end

---Open an iterator over directory entries.
---@param path string Directory path
---@return fun(): string? filename
function fs.dir(path) end

---Open a file handle with specified access mode.
---@param path string Target file
---@param mode "r"|"w"|"a"|"r+"|"w+"|"a+"|"rb"|"wb"|"ab" File open mode
---@return CaisFileHandle|nil handle, string? err
function fs.open(path, mode) end

return fs
