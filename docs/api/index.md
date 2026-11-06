# C API Reference

The **Cais** C API is engineered in pure **ANSI C89 (C90)** with selective GNU89/POSIX/Win32 platform shims. It prioritizes deterministic execution, zero hidden heap allocations in public interfaces, and total portability across contemporary workstations and vintage systems.

---

## Navigation & Indices

* :material-file-code-outline: **[Headers & Subsystems](files.md)** — Modular C header files defining core subsystems, types, and platform abstractions.
* :material-code-json: **[Data Structures](classes.md)** — Canonical C structures including `cais_pollfd_t`, `cais_fs_dirent_t`, `cais_fs_stat_t`, and `cais_serial_opts_t`.
* :material-function: **[Functions Index](functions.md)** — Comprehensive alphabetical index of all C functions exported by the substrate.
* :material-pound: **[Macros & Constants](macros.md)** — Preprocessor constants, error return codes, and bitmask flags.

---

## Subsystem Headers

| Subsystem | Header | Key Functions & Types | Primary Purpose |
| :--- | :--- | :--- | :--- |
| **Core & Status** | [`<cais/types.h>`](types_8h.md) | `cais_status_t`, `cais_bool_t` | Canonical error codes, booleans, and bit-width definitions |
| **Umbrella Lifecycle** | [`<cais/cais.h>`](cais_8h.md) | `cais_init()`, `cais_version()` | Global library initialization, banner, and runtime sanity check |
| **Platform Introspection** | [`<cais/platform.h>`](platform_8h.md) | `cais_platform_os()`, `cais_platform_arch()` | Compile-time and runtime OS, CPU, bitness, and endianness inspection |
| **Filesystem & Path VFS** | [`<cais/filesystem.h>`](filesystem_8h.md) | `cais_fs_open()`, `cais_fs_stat_t`, `cais_fs_dirent_t` | Portable VFS, file streaming, directory traversal, and path normalization |
| **Network Sockets** | [`<cais/socket.h>`](socket_8h.md) | `cais_socket_open()`, `cais_socket_poll()`, `cais_pollfd_t` | Berkeley/Winsock sockets, non-blocking I/O, timeouts, and event multiplexing |
| **Serial Port (UART)** | [`<cais/serial.h>`](serial_8h.md) | `cais_serial_open()`, `cais_serial_opts_t` | RS-232 serial communication with baud, parity, and modem line status |
| **Parallel Port (IEEE 1284)** | [`<cais/parallel.h>`](parallel_8h.md) | `cais_parallel_open()`, `cais_parallel_write()` | Centronics/IEEE 1284 bidirectional strobe and status pin interaction |
| **Kernel Sandboxing** | [`<cais/sandbox.h>`](sandbox_8h.md) | `cais_sandbox_apply()`, `cais_sandbox_policy_t` | OS security containment via Landlock, Pledge, Capsicum, or AppContainer |

---

## Core C89 Design Conventions

### 1. Deterministic Status Codes
All functions with potential runtime errors return `cais_status_t`:

* `CAIS_OK` (`0`): Operation succeeded.
* `CAIS_ERROR` (`-1`): Operation encountered a runtime failure.
* `CAIS_UNSUPPORTED` (`-2`): Subsystem or feature is unavailable on the host kernel.
* `CAIS_EOF` (`-3`): Stream or directory enumeration reached end of entries.
* `CAIS_AGAIN` (`-4`): Non-blocking socket operation would block.

### 2. Caller-Allocated Buffers
Functions returning string diagnostic messages or formatted paths require caller-allocated buffers (`char *out, size_t out_cap`) to prevent hidden memory allocations and ensure thread-safety.

```c
char err[128];
cais_socket_t *sock = cais_socket_open(CAIS_SOCK_STREAM, err, sizeof(err));
if (!sock) {
    fprintf(stderr, "Socket error: %s\n", err);
}
```

### 3. Stack-Allocated Structs
All descriptor and status structures (`cais_pollfd_t`, `cais_fs_stat_t`, `cais_serial_opts_t`) have exposed, fixed layouts that can be safely allocated on the stack without constructor functions.
