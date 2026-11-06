# Building & Compiling

`cais` uses [xmake](https://xmake.io) and [Recudo](https://github.com/projectum-lunare/recudo) as its cross-platform build engine.

---

## Prerequisites

- **xmake** (v2.8.0 or newer)
- Standard C compiler (GCC, Clang, MSVC, MinGW, or Open Watcom)
- *(Optional)* Lua (>= 5.4) if building Lua bindings

---

## Standard Build

Clone the repository and run:

```bash
# Build static library, shared library, and Lua bindings
xmake

# Run the comprehensive C API test suite
xmake test

# Run the Lua automated test suite
xmake run test_lua

# Run the standalone smoke executable
xmake run cais_smoke
```

---

## Modular Targets

If your target system has severe storage or memory limitations (e.g. vintage machines or embedded microcontrollers), you can build individual submodules independently:

```bash
# Build only the C static library:
xmake build cais_static

# Build only standalone Lua submodules:
xmake build cais.platform
xmake build cais.serial
xmake build cais.socket
xmake build cais.fs
xmake build cais.parallel
xmake build cais.sandbox
```

---

## Compile-Time Exclusion Flags

When embedding `cais` directly into proprietary build systems or embedded firmware, unwanted subsystems can be stripped completely via preprocessor defines:

| Preprocessor Flag | Excluded Subsystem |
| :--- | :--- |
| `-DCAIS_NO_SOCKET` | Excludes BSD Sockets, Winsock, and networking code. |
| `-DCAIS_NO_SERIAL` | Excludes RS-232 UART driver and modem controls. |
| `-DCAIS_NO_PARALLEL` | Excludes Centronics parallel port and strobe timing. |
| `-DCAIS_NO_FILESYSTEM` | Excludes file manipulation, directory iterators, and stats. |
| `-DCAIS_NO_SANDBOX` | Excludes Landlock, Pledge, and Capsicum kernel sandboxing. |
| `-DCAIS_NO_PLATFORM` | Excludes OS and CPU architecture detection metadata. |