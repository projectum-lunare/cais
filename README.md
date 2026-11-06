# cais 🌊

> **Communication Abstraction & I/O Substrate**  
> _Part of the [Projectum Lunare](https://github.com/projectum-lunare) Ecosystem._

[![License: MPL-2.0](https://img.shields.io/badge/License-MPL_2.0-blue.svg)](https://mozilla.org/MPL/2.0/)
[![Language: ANSI C89 / GNU89](https://img.shields.io/badge/C-ANSI_C89_%2F_GNU89-brightgreen.svg)]()
[![Lua: 5.4-5.5](https://img.shields.io/badge/Lua-5.4_--_5.5-blue.svg)]()
[![Documentation](https://img.shields.io/badge/Docs-Online-blueviolet.svg)](https://projectum-lunare.github.io/cais/)

`cais` is a portable, zero-bloat system foundation library featuring public C headers and core interfaces designed for strict ANSI C (C89 / ISO C90) interoperability, with implementations and vintage drivers targeting `gnu89` (leveraging standard platform runtime features like `vsnprintf` and GCC extensions such as `__asm__`, struct alignment, and register calling conventions). It bridges hardware I/O, network communication, physical ports, persistent filesystems, and kernel-level sandboxing across **10 distinct operating system families** — from vintage platforms of the 1990s (DOS, Amiga, Windows CE, Atari ST) to modern POSIX workstations in 2026.

---

## 🏛️ Architecture & Core Modules

The library is structured as a cohesive monorepo divided into specialized subsystems, available both as a native C API (`libcais.a` / `libcais.so`) and as Lua modules (`cais.*`):

```text
┌──────────────────────────────────────────────────────────────────────────────────┐
│                           cais (libcais / Lua API)                               │
├──────────────────────────────────────────────────────────────────────────────────┤
│ • cais.platform   │ Compiler, OS & CPU detection, bits & endianness              │
│ • cais.filesystem │ Safe file and directory I/O (alias: cais.fs)                 │
│ • cais.parallel   │ Centronics parallel port (LPT / SPP)                         │
│ • cais.serial     │ RS-232 serial UART with hardware flow control                │
│ • cais.socket     │ Raw TCP & UDP stream/datagram sockets (IPv4)                 │
│ • cais.sandbox    │ Kernel sandboxing (Landlock, Seatbelt, Win32, OpenBSD)       │
│ • cais.noise      │ Noise-based secure transport (XX, NN, NK, IK, 32-bit BLAKE2s)│
│ • cais.tls        │ Modular TLS transport (pluggable backends, custom I/O)        │
└──────────────────────────────────────────────────────────────────────────────────┘
```

All subsystems share a unified canonical type foundation (`include/cais/types.h`) providing strict C89/C90 types with seamless C99/C++ interoperability:

- `cais_status_t`: Canonical I/O and syscall return status (`CAIS_OK`, `CAIS_ERROR`, `CAIS_AGAIN`, `CAIS_EOF`, `CAIS_UNSUPPORTED`).
- `cais_bool_t`: Canonical boolean type (`CAIS_TRUE` = 1, `CAIS_FALSE` = 0) eliminating ambiguous integer truth values.
- `cais_line_state_t`: Physical signal line state (`CAIS_LINE_HIGH`, `CAIS_LINE_LOW`, `CAIS_LINE_ERROR`).

### 1. `platform` — Platform & Environment Foundation

- Comprehensive compile-time and runtime identification of compilers (`gcc`, `clang`, `watcom`, `sasc`, `vbcc`, `msvc`), architectures (`x86_64`, `x86`, `arm`, `arm64`, `m68k`, `ppc`, `mips`, `riscv64`, `wasm32`), bitness, and endianness.
- Pure metadata and environment introspection with zero I/O side-effects.

### 2. `filesystem` (alias `fs`) — Storage & File Operations

- Portable filesystem operations across vintage and modern operating systems without crashing on raw device nodes.
- Type-safe predicates returning canonical `cais_bool_t` (`cais_fs_exists`, `cais_fs_is_file`, `cais_fs_is_dir`, `cais_fs_is_symlink`, `cais_fs_path_is_absolute`).
- Clean separation between storage syscalls and environment metadata.
- Ergonomic alias: accessible interchangeably as `cais.filesystem` or `cais.fs` (both in the module table and via `require("cais.fs")`).

### 3. `parallel` — Centronics Parallel Port (LPT / SPP)

- Direct hardware strobe, busy status check, and buffer drain for impact printers, matrix devices, and parallel links with typed enums (`cais.parallel.mode`, `cais.parallel.pin`, `cais.parallel.port`, `cais.parallel.status`).
- Hardware status monitoring: queries real-time printer status pins (`BUSY`, `ACK`, `PE`, `SLCT`, `FAULT`). `cais_parallel_drain` actively polls the BUSY line until printing completes, returning `CAIS_PARALLEL_OK` or `CAIS_PARALLEL_ERROR` on timeout.
- Standard line printer devices (`/dev/lp*`, `LPT1`) operate in Centronics SPP mode; bidirectional IEEE 1284 modes return `CAIS_PARALLEL_UNSUPPORTED` on unidirectional printer ports.
- Backends: POSIX `/dev/lp0`, Windows NT `LPT1`, Windows CE, DOS `INT 17h` and direct I/O port strobe, Amiga `parallel.device`, Atari ST, KolibriOS, OS/2, Plan 9.

### 4. `serial` — RS-232 Serial UART

- Industrial-grade serial communication with typed enums (`cais.serial.flow`, `cais.serial.parity`, `cais.serial.stop_bits`, `cais.serial.data_bits`, `cais.serial.baud`, `cais.serial.line`, `cais.serial.status`): configurable baud rates, data bits, parity, stop bits, and flow control.
- Safe default: flow control defaults to `NONE` (0). Hardware RTS/CTS is opt-in, preventing write hangs on 3-wire cables or USB adapters without CTS connected.
- Full bidirectional UART: `write()`, `read()`, hardware buffer flush (`cais_serial_drain` returning `cais_serial_status_t`), timeout control (`set_timeout()`), and modem control/status lines (`set_rts`, `set_dtr`, `get_cts`, `get_dsr`, `get_cd`, `get_ri`) returning typed `cais_line_state_t` (`CAIS_LINE_HIGH`, `CAIS_LINE_LOW`, `CAIS_LINE_ERROR`).
- Flexible initialization supporting both ergonomic options tables and classic positional parameters.
- Backends: POSIX termios, Windows NT Comm API, Windows CE, DOS BIOS `INT 14h` / 8250 UART, Amiga `serial.device`, Atari ST, KolibriOS, OS/2, Plan 9.

### 5. `socket` — Raw Network Sockets (IPv4)

- Unified IPv4 TCP & UDP socket abstraction with typed Enums (`cais.socket.type`, `cais.socket.mode`, `cais.socket.event`, `cais.socket.status`).
- Synchronous and asynchronous operation modes (`sock:set_mode`, `sock:set_blocking`, `sock:set_timeout`).
- TCP client connections (`cais.socket.connect`), server binding (`cais.socket.bind`), and client acceptance (`sock:accept`).
- UDP datagram sockets (`cais.socket.udp`, `cais.socket.udp_bind`), datagram transmission (`sock:sendto`), and reception with sender addressing (`sock:recvfrom`).
- Multiplexing beyond `select()` (`cais_socket_poll` in C, `cais.socket.select` in Lua):
  - **Modern POSIX (`poll(2)`):** Native linear array multiplexing for Linux, FreeBSD, OpenBSD, NetBSD, macOS, and Solaris. Eliminates `FD_SETSIZE` (1024) limits and stack corruption risks when handling high descriptor values ($fd \ge 1024$).
  - **Windows (NT 3.51 to 11):** Adaptive runtime multiplexer. Dynamically resolves `WSAPoll` via `GetProcAddress` on Windows Vista..11 / Server 2008..2025 without static DLL import table dependencies. On Windows NT 3.51, 4.0, 2000, and XP, automatically falls back to Winsock `select()` with `FD_SETSIZE` safely expanded to 1024 socket handles (an explicit array in Winsock, avoiding bitmask overflows).
  - **Vintage Non-POSIX BSD Stacks:** Direct native BSD `select()` engine preserving full functionality on MS-DOS (WATT-32 `select_s`), AmigaOS (`bsdsocket.library`), Atari ST (MiNTNet), OS/2, and Plan 9.
  - **Zero-Allocation Fast Path:** Stack-allocated 64-descriptor scratch buffer (`struct pollfd` / `WSAPOLLFD`) for common workloads, eliminating heap allocation on high-frequency I/O loops.
- Backends: BSD Sockets (POSIX, macOS, Linux, BSD), Winsock 2 (Windows NT / CE), WATT-32 (MS-DOS 32-bit), `bsdsocket.library` (AmigaOS m68k), OS/2, Plan 9, KolibriOS.

### 6. `sandbox` — Kernel-Level Sandboxing

- Enforces the principle of least privilege directly inside operating system kernels via unified declarative policies (`cais_sandbox_apply` in C, `cais.sandbox.apply` in Lua):
  - **Linux:** Modern Landlock LSM filesystem and TCP network restriction with `prctl(PR_SET_NO_NEW_PRIVS)`.
  - **macOS (Darwin):** Apple Seatbelt (`sandbox_init`) compiled dynamically from declarative policies into Sandbox Profile Language (SBPL).
  - **Windows NT (10 / 11 / Server):** Process Mitigation Policies (child process creation restriction, dynamic code / W^X enforcement, strict handle checking).
  - **OpenBSD:** Automatic synthesis of `unveil(2)` path rules and `pledge(2)` promises from policy declarations.
  - **FreeBSD:** Capsicum capability mode.
  - **Other platforms:** Safe fallback stub returning `CAIS_SANDBOX_UNSUPPORTED` with zero runtime overhead.

### 7. `noise` — Noise-Based Cryptographic Transport

- Certificate-less, zero-allocation secure channels powered by the Noise Protocol Framework (`Noise_XX`, `Noise_NN`, `Noise_NK`, `Noise_IK` with X25519, ChaCha20-Poly1305, and native 32-bit BLAKE2s).
- Full mutual authentication, identity hiding, WireGuard-style 1-RTT session resumption (`Noise_IK`), and forward secrecy.
- Automatic transport framing over stream sockets (`cais.socket`) and physical serial UART (`cais.serial`).
- Zero heap allocation on common paths and lazy loading in Lua.

### 8. `tls` — Modular TLS Secure Transport

- Pluggable, extensible TLS engine with a modular backend driver architecture (`cais_tls_backend_ops_t`); supported protocol versions, ciphersuites, and features depend on the active driver (such as the lightweight **BearSSL** client driver).
- Decoupled I/O: seamlessly wraps `cais.socket` or accepts arbitrary custom read/write callbacks for serial links, pipes, and in-memory streams.
- Zero runtime heap allocation during packet processing, with support for both strict X.509 certificate validation and lightweight unverified modes (`CAIS_TLS_VERIFY_NONE`) for embedded or constrained environments.
- Non-blocking event loop integration (`CAIS_TLS_WANT_READ`, `CAIS_TLS_WANT_WRITE`) and lazy module loading in Lua.

---

## 🌐 Platform Support Matrix

| Platform Family                   | `platform` | `filesystem` | `parallel`           | `serial`            | `socket`               | `sandbox`            |
| :-------------------------------- | :--------: | :----------- | :------------------- | :------------------ | :--------------------- | :------------------- |
| **Linux (Modern & Embedded)**     |     ✅     | ✅ POSIX     | ✅ `/dev/lp*`        | ✅ termios          | ✅ BSD Sockets         | ✅ Landlock LSM      |
| **OpenBSD / FreeBSD / NetBSD**    |     ✅     | ✅ POSIX     | ✅ `/dev/lpt*`       | ✅ termios          | ✅ BSD Sockets         | ✅ Pledge / Capsicum |
| **macOS (Darwin)**                |     ✅     | ✅ POSIX     | ⚠️ Stub              | ✅ termios          | ✅ BSD Sockets         | ✅ Seatbelt SBPL     |
| **Windows NT (10 / 11 / Server)** |     ✅     | ✅ Win32     | ✅ Win32 LPT         | ✅ Comm API         | ✅ Winsock 2           | ✅ Mitigations       |
| **Windows CE / Pocket PC**        |     ✅     | ✅ WinCE     | ✅ WinCE LPT         | ✅ Comm API         | ✅ Winsock CE          | ⚠️ Stub              |
| **MS-DOS (16/32-bit WATT-32)**    |     ✅     | ✅ DOS io    | ✅ `INT 17h` / I/O   | ✅ `INT 14h` / 8250 | ✅ WATT-32 TCP/IP      | ⚠️ Stub              |
| **AmigaOS (m68k / AROS)**         |     ✅     | ✅ Lock DOS  | ✅ `parallel.device` | ✅ `serial.device`  | ✅ `bsdsocket.library` | ⚠️ Stub              |
| **Atari ST (TOS / MiNT)**         |     ✅     | ✅ Fsfirst   | ✅ Hardware I/O      | ✅ MFP 68901 UART   | ✅ Socket API          | ⚠️ Stub              |
| **KolibriOS**                     |     ✅     | ⚠️ Stub      | ✅ Syscall 46 / I/O  | ✅ Syscall 46 / UART| ✅ Syscall 75          | ⚠️ Stub              |
| **OS/2 / ArcaOS**                 |     ✅     | ✅ access    | ✅ DosOpen LPT       | ✅ DosDevIOCtl      | ✅ TCP/IP Sockets      | ⚠️ Stub              |
| **Plan 9 from Bell Labs**         |     ✅     | ✅ access    | ✅ `/dev/lp`         | ✅ `/dev/eia*`      | ✅ `/net/tcp`          | ⚠️ Stub              |

> [!TIP]
> For the comprehensive breakdown of minimum operating system versions and runtime dynamic degradation guarantees (from Windows NT 3.1 in 1993 to Linux 6.12+ in 2026), see the [Platform Architecture & Support Directory](https://projectum-lunare.github.io/cais/platforms/).

---

## 🚀 Quickstart: Lua API

```lua
local cais = require("cais")

-- 1. Platform Detection
print("Running on: " .. cais.platform.os)
print("Architecture: " .. cais.platform.arch)
print("Is 64-bit: " .. tostring(cais.platform.is_64bit))

-- 2. Filesystem (usable as cais.filesystem or cais.fs)
if cais.fs.exists("/etc/hosts") then
    print("Found hosts file via cais.fs!")
end

-- 3. Kernel Sandboxing (OOP builder or declarative table)
local backend, version = cais.sandbox.backend()
print("Sandbox backend: " .. backend)

-- Restrict filesystem & network via fluent method chaining:
cais.sandbox.policy()
    :allow(".", "r")
    :deny_network()
    :apply()

-- 4. Network Sockets (socket)
local client, err = cais.socket.connect("127.0.0.1", 9100)
if client then
    client:write("Print payload via cais!\n")
    client:close()
end

-- 5. Serial Port (serial)
local port, err = cais.serial.open("/dev/ttyUSB0", {
    baud = cais.serial.baud.B115200,
    data_bits = 8,
    parity = "none",
    stop_bits = 1,
    flow = cais.serial.flow.HARDWARE
})
if port then
    port:write("AT\r\n")
    port:drain()
    port:close()
end

-- 6. Parallel Port (parallel / Centronics SPP)
local lpt, err = cais.parallel.open(cais.parallel.port.LPT1)
if lpt then
    lpt:write("Print payload via Centronics!\n\f")
    lpt:drain()
    lpt:close()
end
```

---

## 🔨 Quickstart: C API

```c
#include <stdio.h>
#include <cais/cais.h>

int main(void) {
    char err_buf[128];
    cais_socket_t *sock;
    size_t sent = 0;

    printf("cais version: %s\n", cais_version());
    printf("OS: %s, Arch: %s\n", cais_platform_os(), cais_platform_arch());

    /* Canonical boolean check */
    if (cais_fs_exists("/etc/hosts") == CAIS_TRUE) {
        printf("Host configuration exists on disk.\n");
    }

    /* Connect to raw TCP socket */
    sock = cais_socket_connect("127.0.0.1", 1993, CAIS_SOCK_BLOCKING, err_buf, sizeof(err_buf));
    if (!sock) {
        fprintf(stderr, "Connection failed: %s\n", err_buf);
        return 1;
    }

    if (cais_socket_send(sock, "GET / HTTP/1.0\r\n\r\n", 18, &sent) == CAIS_SOCK_OK) {
        printf("Sent %lu bytes\n", (unsigned long)sent);
    }
    cais_socket_close(sock);
    return 0;
}
```

---

## 📦 Building and Testing

`cais` public C headers (`include/cais/*.h`) and consumer-facing C code are strictly ANSI C89 (C90) compliant, ensuring clean interoperability across vintage and modern C compilers. Internal library implementations and platform-specific drivers (such as AmigaOS m68k, Atari ST TOS/MiNT, and KolibriOS) compile under `gnu89` where compiler extensions (`__asm__`, struct alignment, `#pragma`) and standard C-runtime features (such as `vsnprintf`) are utilized.

The project uses [xmake](https://xmake.io) and [Recudo](https://github.com/projectum-lunare/recudo) as its unified build system.

```bash
# Build static library, shared library, and Lua module:
xmake

# Run C API unit tests:
xmake test

# Run Lua test suite via Recudo launcher:
xmake run test_lua

# Run minimal standalone smoke executable:
xmake run cais_smoke

# Clean artifacts:
xmake clean
```

### 📚 Documentation

The official documentation is available online at:  
👉 **[https://projectum-lunare.github.io/cais/](https://projectum-lunare.github.io/cais/)**

To build and serve the documentation locally using [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/):

```bash
# Install documentation dependencies:
pip install -r requirements-docs.txt

# Start local live-reload documentation server:
mkdocs serve

# Build static production site:
mkdocs build
```

### Modular Targets & Submodules

`cais` builds both an umbrella library (`libcais.a` / `cais.so`) and independent, zero-dependency submodules that can be required in Lua without overhead:

```bash
# Build only a specific submodule or target:
xmake build cais_static
xmake build cais.serial
xmake build cais_smoke
```

#### C Preprocessor Flags

If embedding `cais` directly in a custom build system or firmware, unneeded subsystems can be excluded via compile-time defines:

- `-DCAIS_NO_SOCKET`: Exclude networking and TCP/UDP sockets.
- `-DCAIS_NO_SERIAL`: Exclude serial port abstractions.
- `-DCAIS_NO_PARALLEL`: Exclude parallel port abstractions.
- `-DCAIS_NO_FILESYSTEM`: Exclude filesystem abstractions.
- `-DCAIS_NO_SANDBOX`: Exclude OS kernel sandboxing.
- `-DCAIS_NO_PLATFORM`: Exclude platform metadata.
- `-DCAIS_NO_NOISE`: Exclude Noise cryptographic transport.
- `-DCAIS_NO_TLS`: Exclude TLS secure transport subsystem.

#### Standalone Lua Submodules

Submodules can be required independently without loading the umbrella library or its dependencies:

```lua
local platform = require("cais.platform") -- only platform detection (~17 KB)
local serial   = require("cais.serial")   -- only serial communication (~27 KB)
```

The umbrella `cais` module itself is **lazy**: `require("cais")` only registers subsystem openers in `package.preload`, and each subsystem initializes on first field access — the Lua equivalent of dead-code elimination. `local cais = require("cais")` is nearly free, `cais.socket` loads only when used, and the `cais.fs` alias shares the `cais.filesystem` table identity.

By default (`--kind=static`) the Lua bindings compile as embedding archives under `build/lua_static/` — `libcais_lua.a` and `libcais_<module>_lua.a` carry only the binding objects, so the embedding application brings its own interpreter — and `xmake run`/`xmake test` yield a single self-contained binary with the interpreter, bindings and entry script inside. With `--kind=shared` the modules become dynamic `.so` files linked against the shared Lua runtime (~31 KB each, e.g. `cais.serial`). In both modes the umbrella stays lazy and interpreter symbols stay hidden from the dynamic table.

### Cross-Platform Multi-Target Builds (Recudo)

`cais` uses [Recudo](https://github.com/projectum-lunare/recudo) for automated cross-compilation and platform management:

```bash
# Inspect SDK availability and platform toolchain status:
xmake recudo list

# Build a specific target static library:
xmake recudo posix     # build/posix/libcais.a
xmake recudo dos       # build/dos/libcais.a (Open Watcom + WATT-32)
xmake recudo os2       # build/os2/libcais.a (Open Watcom OS/2)
xmake recudo wince     # build/windows_ce/arm/libcais.a (MS eVC 3.0 ARM via wibo)
xmake recudo amiga     # build/amiga/libcais.a (m68k-amigaos-gcc)
xmake recudo atari     # build/atarist/libcais.a (m68k-atari-mint-gcc)
xmake recudo kolibri   # build/kolibri/libcais.a (kos32-gcc)

# Build all available platforms in a multi-target matrix:
xmake recudo all
```

---

## 📜 License

Mozilla Public License Version 2.0 (MPL-2.0).  
Copyright (C) 2026 Gustavo Veiga.
