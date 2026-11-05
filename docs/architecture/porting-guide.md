# Platform Abstraction Layer (PAL) Porting Guide

This guide details how to extend `cais` to support a **new operating system or kernel target** (such as QNX, Zephyr RTOS, Haiku native APIs, or Redox OS).

---

## 1. Architecture Overview

`cais` strictly decouples public consumer-facing headers from internal platform-specific drivers:

```
[ Application Code ] ──> include/cais/*.h (Strict ANSI C89)
                             │
                             ▼
                     src/*/*_driver.c (gnu89 implementation)
                             │
                             ▼
                     src/*/*_pal.h (Platform Abstraction Layer)
                             │
    ┌────────────────┬───────┴────────┬────────────────┐
    ▼                ▼                ▼                ▼
[ POSIX PAL ]  [ Win32 PAL ]    [ DOS/Amiga PAL ]  [ YOUR NEW PAL ]
```

### The Cardinal Rules of Porting:
1. **Never pollute `include/cais/*.h`:** Public headers must never include OS-specific headers (such as `<windows.h>`, `<sys/socket.h>`, `<dos.h>`, or RTOS headers).
2. **Strict ANSI C89 in Public APIs:** All structs, typedefs, and function prototypes visible to consumers must compile under C89 (`-std=c89`).
3. **`gnu89` Scoping for Drivers:** Driver implementation files (`*_driver.c`) may use GCC compiler extensions (`__asm__`, inline assembly, `#pragma`, register calling conventions).

---

## 2. Step-by-Step Porting Checklist

### Step 1: Define the Platform Identifier

In [`src/platform/platform_pal.h`](file:///home/gustavo/Projetos/cais/src/platform):
1. Add compile-time detection for your OS:
   ```c
   #elif defined(__QNX__) || defined(__QNXNTO__)
       #define CAIS_IS_QNX 1
       #define CAIS_IS_POSIX 1
   ```
2. In [`src/platform/platform.c`](file:///home/gustavo/Projetos/cais/src/platform/platform.c), register the string identifier:
   ```c
   #if CAIS_IS_QNX
       return "qnx";
   #endif
   ```
3. In [`src/platform/platform_lua.c`](file:///home/gustavo/Projetos/cais/src/platform/platform_lua.c), register the boolean flag:
   ```c
   lua_pushboolean(L, CAIS_IS_QNX ? 1 : 0);
   lua_setfield(L, -2, "is_qnx");
   ```

---

### Step 2: Implement the Socket PAL

In [`src/socket/socket_pal.h`](file:///home/gustavo/Projetos/cais/src/socket/socket_pal.h):
1. Define the native socket handle type:
   ```c
   #define CAIS_PAL_SOCK_T int
   #define CAIS_PAL_INVALID_SOCKET (-1)
   #define CAIS_SOCK_IS_VALID(s) ((s) != NULL && (s)->fd >= 0)
   ```
2. Map interrupt errors:
   ```c
   #define CAIS_PAL_IS_INTR(e) ((e) == EINTR)
   ```
3. Map non-blocking would-block checks:
   ```c
   #define CAIS_PAL_IS_WOULDBLOCK(e) ((e) == EAGAIN || (e) == EWOULDBLOCK)
   ```
4. Implement `cais_pal_strerror` to format native error numbers without heap allocations.

---

### Step 3: Implement the Serial UART Driver

In [`src/serial/serial_pal.h`](file:///home/gustavo/Projetos/cais/src/serial/serial_pal.h):
1. Define the internal `struct serial_port`:
   ```c
   struct serial_port {
       int fd;             /* File descriptor or hardware handle */
       int timeout_ms;     /* Current read/write timeout */
       /* OS-specific termios or register state */
   };
   ```
2. Implement the mandatory PAL lifecycle primitives:
   * `cais_pal_serial_open()`
   * `cais_pal_serial_close()`
   * `cais_pal_serial_read()`
   * `cais_pal_serial_write()`
   * `cais_pal_serial_drain()`
   * `cais_pal_serial_set_timeout()`
   * `cais_pal_serial_set_rts()` / `cais_pal_serial_set_dtr()`
   * `cais_pal_serial_get_cts()` / `cais_pal_serial_get_dsr()`

---

### Step 4: Implement Filesystem PAL

In [`src/filesystem/filesystem_pal.h`](file:///home/gustavo/Projetos/cais/src/filesystem/filesystem_pal.h):
* Implement stat inspection, directory iteration (`opendir`/`readdir` or native equivalent), and directory tree manipulation.
* Ensure path algebra functions (`join`, `normalize`, `dirname`, `basename`) remain pure string manipulations with **zero syscalls**.

---

### Step 5: Implement Kernel Sandboxing (or Fallback)

If your OS does not have a native kernel sandbox (such as Linux Landlock or OpenBSD Pledge), `cais` provides a standardized fallback:

In [`src/sandbox/sandbox_pal.h`](file:///home/gustavo/Projetos/cais/src/sandbox/sandbox_pal.h):
```c
#if CAIS_IS_YOUR_OS
    #define CAIS_PAL_SANDBOX_BACKEND CAIS_SANDBOX_BACKEND_NONE
    #define CAIS_PAL_SANDBOX_NAME    "none"
#endif
```

All functions will cleanly return `CAIS_SANDBOX_UNSUPPORTED` (`-4`), allowing applications to gracefully detect that the platform lacks hardware containment without aborting.

---

### Step 6: Add Cross-Compilation Target to Recudo

In [`xmake.lua`](file:///home/gustavo/Projetos/cais/xmake.lua):
Register your toolchain in Recudo:
```lua
target("cais_qnx")
    set_kind("static")
    set_toolchains("qnx-gcc")
    add_defines("CAIS_IS_QNX=1")
    add_files("src/**.c")
```
