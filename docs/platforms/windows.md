# Windows (NT 3.1..11, CE)

`cais` maintains runtime binary and source compatibility across 30 years of Windows versions, from **Windows NT 3.1 (1993)** and **Windows NT 3.51 (1995)** to modern **Windows 11 (2026)** and **Windows CE 3.0 / Pocket PC**.

---

## 🏛️ Windows Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All Releases)

The following core subsystems are **universally available** across all supported versions of Windows (from NT 3.1/3.51 to Windows 11 and Windows CE):

* **`cais.platform`:** Win32 & CE architecture, CPU bitness, and endianness predicates.
* **`cais.filesystem`:** Full file I/O (`CreateFileA` on NT; transparent UTF-16 `*W` conversions on CE).
* **`cais.parallel`:** Native IEEE 1284 parallel printer ports (`LPT1`..`LPT3`).
* **`cais.serial`:** Standard Win32 Communications API (`COM1`..`COM9`, `\\.\COM10+`).

---

### 🪟 Subsystem Support by Windows Generation

#### 🔹 Modern Windows (Windows 10, 11 & Server 2016..2025)

* **Kernel:** NT 10.0 | **Architectures:** x86, x86_64, ARM64
* **Network Sockets (`cais.socket`):** ✅ Native dynamic `WSAPoll` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Full Process Mitigation Policies:
  * Prohibits child process creation (`ProcessChildProcessPolicy`, Windows 10 Version 1607+).
  * Prohibits dynamic executable memory allocations (W^X / `ProcessDynamicCodePolicy`).
  * Enforces strict handle checking (`ProcessStrictHandleCheckPolicy`).

#### 🔹 Intermediate Windows (Windows 8 & 8.1 / Server 2012 & 2012 R2)

* **Kernel:** NT 6.2 / 6.3 | **Architectures:** x86, x86_64
* **Network Sockets (`cais.socket`):** ✅ Native dynamic `WSAPoll` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** 🟡 Partial Process Mitigations:
  * Enforces W^X dynamic code hardening and strict handle checking.
  * Child process restrictions not yet available in kernel (degrades cleanly without error).

#### 🔹 Legacy Windows (Windows Vista & 7 / Server 2008 & 2008 R2)

* **Kernel:** NT 6.0 / 6.1 | **Architectures:** x86, x86_64
* **Network Sockets (`cais.socket`):** ✅ Native dynamic `WSAPoll` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** ⚠️ Safe fallback stub (returns `CAIS_SANDBOX_UNSUPPORTED` / `-4`).

#### 🔹 Vintage Windows NT (Windows NT 3.51, 4.0, 2000, XP / Server 2003)

* **Kernel:** NT 3.51, 4.0, 5.0, 5.1, 5.2 | **Architectures:** x86, Alpha, MIPS, PowerPC
* **Network Sockets (`cais.socket`):** ✅ Winsock `select()` with array expanded to 1024 handles (`FD_SETSIZE 1024`).
* **Kernel Sandboxing (`cais.sandbox`):** ⚠️ Safe fallback stub (returns `CAIS_SANDBOX_UNSUPPORTED` / `-4`).

#### 🔹 Embedded Windows (Windows CE 3.0..6.0 / Pocket PC)

* **Kernel:** Windows CE | **Architectures:** ARM, MIPS, SH3/SH4, x86
* **Filesystem (`cais.filesystem`):** ✅ Transparent UTF-16 `*W` Win32 API bridge.
* **Network Sockets (`cais.socket`):** ✅ Winsock CE `select()` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** ⚠️ Safe fallback stub (returns `CAIS_SANDBOX_UNSUPPORTED` / `-4`).

---

## 🔒 Kernel Sandboxing & Process Mitigations (`cais.sandbox`)

On Windows, `cais.sandbox` leverages native kernel-level **Process Mitigation Policies** via `SetProcessMitigationPolicy`.

### Supported Security Mitigations

1. **Child Process Restriction (`ProcessChildProcessPolicy`):**
   * Blocks the current process and its descendants from spawning any new processes (`CreateProcess` fails with `ERROR_ACCESS_DENIED`).
   * *Introduced in:* Windows 10 Version 1607 (Anniversary Update / Redstone 1) and Windows Server 2016.
2. **Dynamic Code Hardening (`ProcessDynamicCodePolicy`):**
   * Enforces W^X (Write XOR Execute) memory semantics: prohibits allocating `PAGE_EXECUTE_READWRITE` pages or converting existing data pages into executable code.
   * *Introduced in:* Windows 8.1 / Windows 10.
3. **Strict Handle Checking (`ProcessStrictHandleCheckPolicy`):**
   * Raises an immediate structured exception if the process attempts to access an invalid or closed handle.
   * *Introduced in:* Windows 8.1 / Windows 10.

### Vintage PE Loader Safety Guarantee

Calling `SetProcessMitigationPolicy` statically would bake an import reference into `KERNEL32.DLL` that prevents the Windows PE loader from launching the binary on Windows 7, XP, 2000, or NT 4.0.

To ensure strict backward compatibility:

1. `SetProcessMitigationPolicy` is resolved dynamically at runtime via `GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetProcessMitigationPolicy")`.
2. On Windows 10 and 11, the function is located and security policies are enforced at the NT kernel level.
3. On Windows 7, Vista, XP, and earlier, `GetProcAddress` returns `NULL`. `cais_sandbox_is_supported()` evaluates to `CAIS_FALSE`, and calling `cais_sandbox_apply()` returns `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with an explanatory error message. The application continues running without loader errors or crashes.

---

## 🌐 Dynamic Socket Multiplexing: `WSAPoll` vs. `select()`

`WSAPoll` was introduced in Windows Vista (NT 6.0). Static linkage against `WSAPoll` prevents executables from starting on Windows NT 3.51 through Windows XP.

### Adaptive Resolution

1. The runtime engine probes `WSAPoll` dynamically via `GetProcAddress(GetModuleHandleA("ws2_32.dll"), "WSAPoll")`.
2. **Windows Vista through 11:** Invokes native `WSAPoll`, avoiding `select()` bitmap limits.
3. **Windows NT 3.51 through XP:** Automatically falls back to Winsock `select()`.

### Winsock `FD_SETSIZE` Expansion (1024 Handles)

Unlike POSIX (where `fd_set` is a fixed bitmask and redefining `FD_SETSIZE` causes stack/heap buffer overflows), Winsock defines `fd_set` as an explicit array of socket handles:

```c
typedef struct fd_set {
    u_int   fd_count;
    SOCKET  fd_array[FD_SETSIZE];
} fd_set;
```

`cais` defines `#define FD_SETSIZE 1024` prior to including `<winsock2.h>`. This enables the fallback `select()` engine on Windows NT 3.51, 4.0, 2000, and XP to monitor up to 1024 concurrent socket descriptors without memory corruption.

---

## 💾 Filesystem & Unicode on Windows CE

On standard Windows NT, filesystem paths use the standard ANSI (`A`) or Wide (`W`) Win32 APIs. On Windows CE / Pocket PC, Microsoft omitted the ANSI entry points (`CreateFileA`, `GetFileAttributesA`).

In [`src/filesystem/filesystem_windows_ce.c`](file:///home/gustavo/Projetos/cais/src/filesystem/filesystem_windows_ce.c), `cais` transparently converts UTF-8 / ANSI paths to wide character buffers using `MultiByteToWideChar(CP_ACP, ...)` before calling wide Win32 APIs (`GetFileAttributesW`, `FindFirstFileW`), ensuring seamless file I/O on embedded devices.

---

## ⏱️ Version Milestones

| Feature                                 |           Minimum Windows Version           | Description & Fallback Behavior                                                                                                              |
| :-------------------------------------- | :-----------------------------------------: | :------------------------------------------------------------------------------------------------------------------------------------------- |
| **Core Architecture & Platform**        |           Windows NT 3.1+ (1993)            | Base 32-bit Win32 execution, CPU architecture detection, and endianness predicates                                                           |
| **Filesystem & Comm API (Serial/LPT)**  |           Windows NT 3.51+ (1995)           | Complete Win32 file manipulation (`CreateFileA`), standard COM serial ports, and LPT parallel printer ports                                  |
| **Expanded Winsock `select()`**         |           Windows NT 3.51+ (1995)           | Asynchronous socket multiplexing with array expanded to 1024 descriptors (`FD_SETSIZE 1024`)                                                 |
| **Windows CE Unicode Filesystem**       |           Windows CE 3.0+ (2000)            | Transparent ANSI/UTF-8 to UTF-16 (`*W`) conversion for Pocket PC and Windows Mobile devices                                                  |
| **`WSAPoll` Socket Multiplexing**       |    Windows Vista / Server 2008 (NT 6.0+)    | Dynamic linear polling via `WSAPoll`; on earlier versions (XP, 2000, NT), automatically falls back at runtime to `select()` (1024 handles)   |
| **Process Mitigations (W^X & Handles)** |   Windows 8.1 / Server 2012 R2 (NT 6.3+)    | Kernel-enforced dynamic code restriction (no `PAGE_EXECUTE_READWRITE`) and strict handle checking via dynamic `SetProcessMitigationPolicy`   |
| **Child Process Restriction**           | Windows 10 Version 1607+ (Redstone 1, 2016) | Total lockdown against spawning subprocesses (`ProcessChildProcessPolicy`); cleanly reports unsupported on earlier releases without crashing |
