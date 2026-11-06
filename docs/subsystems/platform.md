# Platform Detection (`cais.platform`)

The `platform` subsystem provides compile-time and runtime introspection of operating system families, CPU architectures, pointer bitness, memory endianness, and compiler toolchains.

It serves as the **foundational layer** of `cais`, enabling applications and higher-level subsystems to make zero-overhead branching decisions across more than **10 operating system families** (from 16-bit MS-DOS to modern 64-bit POSIX and Windows).

---

## ⚡ 1. The Zero-Overhead Guarantee

Unlike traditional platform detection libraries that query host APIs at runtime, `cais.platform` is designed with strict performance guarantees:

1. **Zero System Calls:**
   Functions like `cais_platform_os()` and `cais_platform_arch()` return static string literals embedded directly into the binary at compile time. It never calls `uname(2)`, `GetVersionEx()`, or queries kernel sysctls.
2. **Zero Dynamic Memory Allocations:**
   The subsystem performs zero calls to `malloc()` or `free()`. Memory footprint is completely static.
3. **Compile-Time Dead-Code Elimination:**
   In C, all boolean flags (`CAIS_IS_POSIX`, `CAIS_IS_WINDOWS`, `CAIS_IS_64BIT`, `CAIS_IS_LITTLE_ENDIAN`) are defined as constant integer literals (`1` or `0`). Modern compilers completely optimize away inactive branches inside `#if` or `if (CAIS_IS_...)` blocks.
4. **Immutable Lua Table:**
   When required in Lua (`require("cais.platform")`), the module exposes a frozen, read-only table of strings and booleans materialized once during library initialization.

---

## 🏛️ 2. Core Identifiers & Metadata

### 1. Operating System (`platform.os` / `cais_platform_os()`)
Returns a canonical lowercase identifier string representing the target OS:

| OS Identifier | Target Operating System Family                                |
| :------------ | :------------------------------------------------------------ |
| `"linux"`     | Linux distributions (glibc, musl, uClibc, Android bionic)     |
| `"darwin"`    | Apple Darwin, macOS, iOS, iPadOS                              |
| `"windows"`   | Microsoft Windows NT (Windows NT 3.51..11, Server 2003..2025) |
| `"wince"`     | Microsoft Windows CE 3.0..6.0, Pocket PC, Windows Mobile      |
| `"openbsd"`   | OpenBSD (all architectures)                                   |
| `"freebsd"`   | FreeBSD (all branches)                                        |
| `"netbsd"`    | NetBSD (all architectures)                                    |
| `"dos"`       | MS-DOS, FreeDOS, PC-DOS (16-bit Real Mode & 32-bit DPMI)      |
| `"amiga"`     | Commodore AmigaOS Classic (Motorola 68k)                      |
| `"aros"`      | AROS Research Operating System                                |
| `"amigaos4"`  | AmigaOS 4 (PowerPC)                                           |
| `"atarist"`   | Atari ST, TT, Falcon (TOS, FreeMiNT)                          |
| `"kolibri"`   | KolibriOS (32-bit x86 assembly OS)                            |
| `"os2"`       | IBM OS/2 2.0..Warp 4.52, ArcaOS, eComStation                  |
| `"plan9"`     | Plan 9 from Bell Labs, 9front                                 |
| `"haiku"`     | Haiku OS                                                      |
| `"beos"`      | BeOS (R5 / BONE)                                              |
| `"solaris"`   | Oracle Solaris, OpenIndiana, illumos                          |
| `"minix"`     | Minix 3                                                       |
| `"wasi"`      | WebAssembly System Interface                                  |

---

### 2. CPU Architecture (`platform.arch` / `cais_platform_arch()`)
Returns the normalized processor architecture:

| Architecture String | CPU Family & Variants                                       |
| :------------------ | :---------------------------------------------------------- |
| `"x86_64"`          | AMD64, Intel 64, x86-64                                     |
| `"x86"`             | Intel 8086, 80286, 80386, 80486, Pentium, i686              |
| `"arm64"`           | AArch64, ARMv8-A, ARMv9-A, Apple Silicon (M1..M4)           |
| `"arm"`             | ARMv4T..ARMv7-A (32-bit ARM, armhf, armel)                  |
| `"m68k"`            | Motorola 68000, 68010, 68020, 68030, 68040, 68060, ColdFire |
| `"ppc"`             | PowerPC 32/64-bit (G3, G4, G5, POWER)                       |
| `"mips"`            | MIPS I..V, MIPS32, MIPS64                                   |
| `"riscv64"`         | RISC-V 64-bit (`__riscv_xlen == 64`)                        |
| `"alpha"`           | DEC Alpha AXP 64-bit                                        |
| `"ia64"`            | Intel Itanium IA-64                                         |
| `"sh"`              | Hitachi SuperH (SH-3, SH-4)                                 |
| `"wasm32"`          | WebAssembly 32-bit linear memory                            |

---

### 3. Word Size & Pointer Bitness (`platform.bits` / `cais_platform_bits()`)
Returns an integer representing pointer bitness on the target platform:

* **`16`:** Vintage 16-bit real-mode architectures (MS-DOS 8086/80286).
* **`32`:** Standard 32-bit flat memory architectures (x86, ARM32, m68k, Windows CE, OS/2).
* **`64`:** Modern 64-bit architectures (x86_64, aarch64, riscv64, ppc64).

---

### 4. Memory Endianness (`platform.endian` / `cais_platform_endian()`)
Returns the byte order used by the host CPU when storing multi-byte words in memory:

* **`"little"`:** Least significant byte first (Intel x86/x64, ARM Little-Endian, RISC-V, WASM).
* **`"big"`:** Most significant byte first (Motorola 68k, PowerPC, SPARC, IBM S/390).

---

## 🚩 3. Canonical Boolean Predicates

To eliminate tedious string comparisons in user code, `cais` exposes boolean flags both as preprocessor macros in C and as boolean properties in Lua:

| C Macro Flag            | Lua Property                |    Category     | Description                                               |
| :---------------------- | :-------------------------- | :-------------: | :-------------------------------------------------------- |
| `CAIS_IS_POSIX`         | `platform.is_posix`         | Family Umbrella | `true` on Linux, macOS, BSDs, Haiku, Solaris, Minix       |
| `CAIS_IS_WINDOWS`       | `platform.is_windows`       | Family Umbrella | `true` on all Windows NT and Windows CE systems           |
| `CAIS_IS_WINDOWS_NT`    | `platform.is_windows_nt`    |   OS Specific   | `true` specifically on desktop/server Windows NT          |
| `CAIS_IS_WINDOWS_CE`    | `platform.is_windows_ce`    |   OS Specific   | `true` specifically on Windows CE / Pocket PC             |
| `CAIS_IS_LINUX`         | `platform.is_linux`         |   OS Specific   | `true` on Linux kernels (glibc, musl, uClibc, Android)    |
| `CAIS_IS_DARWIN`        | `platform.is_darwin`        |   OS Specific   | `true` on macOS Darwin (Intel, Apple Silicon, PowerPC)    |
| `CAIS_IS_BSD`           | `platform.is_bsd`           | Family Umbrella | `true` on FreeBSD, OpenBSD, NetBSD, DragonFly BSD         |
| `CAIS_IS_AMIGA`         | `platform.is_amiga`         | Family Umbrella | `true` on AmigaOS Classic, AROS, and AmigaOS 4            |
| `CAIS_IS_DOS`           | `platform.is_dos`           |   OS Specific   | `true` on MS-DOS, FreeDOS, and PC-DOS                     |
| `CAIS_IS_ATARIST`       | `platform.is_atarist`       |   OS Specific   | `true` on Atari ST (TOS, FreeMiNT)                        |
| `CAIS_IS_KOLIBRI`       | `platform.is_kolibri`       |   OS Specific   | `true` on KolibriOS                                       |
| `CAIS_IS_OS2`           | `platform.is_os2`           |   OS Specific   | `true` on IBM OS/2 Warp and ArcaOS                        |
| `CAIS_IS_PLAN9`         | `platform.is_plan9`         |   OS Specific   | `true` on Plan 9 from Bell Labs and 9front                |
| `CAIS_IS_64BIT`         | `platform.is_64bit`         |    Word Size    | `true` if pointers are 64-bit wide (`sizeof(void*) == 8`) |
| `CAIS_IS_32BIT`         | `platform.is_32bit`         |    Word Size    | `true` if pointers are 32-bit wide (`sizeof(void*) == 4`) |
| `CAIS_IS_16BIT`         | `platform.is_16bit`         |    Word Size    | `true` if pointers are 16-bit wide (`sizeof(void*) == 2`) |
| `CAIS_IS_LITTLE_ENDIAN` | `platform.is_little_endian` |   Endianness    | `true` if host byte order is Little-Endian                |
| `CAIS_IS_BIG_ENDIAN`    | `platform.is_big_endian`    |   Endianness    | `true` if host byte order is Big-Endian                   |

> [!NOTE]
> `is_little_endian` and `is_big_endian` are **strictly mutually exclusive**: exactly one evaluates to `true` on any running target.

---

## 🛠️ 4. Compiler Introspection (C API)

[`include/cais/platform.h`](file:///home/gustavo/Projetos/cais/include/cais/platform.h) normalizes compiler differences, exposing compiler family identifiers:

| Compiler Macro    | Identifier String (`CAIS_CC_NAME`) | Compiler Family                                |
| :---------------- | :--------------------------------: | :--------------------------------------------- |
| `CAIS_CC_GCC`     |              `"gcc"`               | GNU Compiler Collection (GCC)                  |
| `CAIS_CC_CLANG`   |             `"clang"`              | LLVM Clang                                     |
| `CAIS_CC_MSVC`    |              `"msvc"`              | Microsoft Visual C++                           |
| `CAIS_CC_WATCOM`  |             `"watcom"`             | Open Watcom C/C++ (DOS, OS/2, Win32)           |
| `CAIS_CC_BORLAND` |            `"borland"`             | Borland Turbo C / C++                          |
| `CAIS_CC_VBCC`    |              `"vbcc"`              | Volker Barthelmann's C Compiler (Amiga, Atari) |
| `CAIS_CC_SAS`     |              `"sasc"`              | SAS/C for Commodore Amiga                      |
| `CAIS_CC_TCC`     |              `"tcc"`               | Fabrice Bellard's Tiny C Compiler              |

It also supplies cross-compiler portable helper macros:
* **`CAIS_INLINE`:** Maps to `static __inline__` on GCC/Clang, `static __inline` on MSVC/Watcom, and `static` on pure ANSI C89 compilers.
* **`CAIS_UNUSED(x)`:** Portable cast `((void)(x))` suppressing compiler warnings on unused parameters.
* **`cais_strdup(s)`:** Clean, safe replacement for POSIX `strdup()` on vintage systems lacking the function.

---

## 💻 5. Practical C89 Programming Examples

### Example 1: Zero-Cost Compile-Time Branching & Endianness Swap
In performance-sensitive networking and file serialization, you need to convert 32-bit integers to Big-Endian network byte order. `cais.platform` eliminates runtime branch overhead:

```c
#include <cais/platform.h>
#include <stdio.h>

/* Converts a 32-bit integer to Network Byte Order (Big Endian) */
CAIS_INLINE unsigned long to_network_u32(unsigned long val) {
#if CAIS_IS_BIG_ENDIAN
    /* On Big-Endian hardware (Amiga m68k, PowerPC), byte order is already native */
    return val;
#else
    /* On Little-Endian hardware (x86, ARM), swap bytes */
    return ((val & 0x000000FFUL) << 24) |
           ((val & 0x0000FF00UL) << 8)  |
           ((val & 0x00FF0000UL) >> 8)  |
           ((val & 0xFF000000UL) >> 24);
#endif
}

int main(void) {
    unsigned long local_val = 0x12345678UL;
    unsigned long net_val   = to_network_u32(local_val);

    printf("Host Operating System: %s\n", cais_platform_os());
    printf("CPU Architecture:      %s\n", cais_platform_arch());
    printf("Pointer Bitness:       %d-bit\n", cais_platform_bits());
    printf("Memory Endianness:     %s\n", cais_platform_endian());

    printf("Original: 0x%08lX -> Network: 0x%08lX\n", local_val, net_val);
    return 0;
}
```

---

### Example 2: Adaptive Memory Sizing for 16-bit vs 32/64-bit Systems
When writing software that must run on 16-bit DOS (where a single data segment cannot exceed 64 KB) as well as modern 64-bit workstations:

```c
#include <cais/platform.h>
#include <stdio.h>
#include <stdlib.h>

#if CAIS_IS_16BIT
    /* 16-bit DOS real-mode: conservative 8 KB streaming buffer */
    #define BUFFER_CAPACITY (8 * 1024)
#elif CAIS_IS_32BIT
    /* 32-bit vintage/embedded: 64 KB buffer */
    #define BUFFER_CAPACITY (64 * 1024)
#else
    /* Modern 64-bit workstation: 1 MB streaming buffer */
    #define BUFFER_CAPACITY (1024 * 1024)
#endif

void allocate_scratch_buffer(void) {
    void *buf = malloc(BUFFER_CAPACITY);
    if (!buf) {
        fprintf(stderr, "Out of memory!\n");
        return;
    }
    printf("Allocated %lu bytes optimized for %d-bit architecture\n", 
           (unsigned long)BUFFER_CAPACITY, cais_platform_bits());
    free(buf);
}
```

---

## 🌙 6. Practical Lua Programming Examples (Lua >= 5.4)

The Lua module is loaded via `require("cais.platform")` or through the umbrella table `cais.platform`.

### Example 1: Cross-Platform File Paths & Temporary Storage Dispatcher
Operating systems use radically different conventions for temporary directories and path separators:

```lua
local platform = require("cais.platform")

local function get_temp_directory()
    if platform.is_windows then
        return os.getenv("TEMP") or os.getenv("TMP") or "C:\\TEMP"
    elseif platform.is_amiga then
        return "T:" -- Standard Amiga RAM temporary assign
    elseif platform.is_dos then
        return os.getenv("TEMP") or "C:\\DOS"
    elseif platform.is_plan9 then
        return "/tmp"
    else
        -- POSIX / macOS / Linux / BSD
        return os.getenv("TMPDIR") or "/tmp"
    end
end

local function get_path_separator()
    if platform.is_windows or platform.is_dos or platform.is_os2 then
        return "\\"
    elseif platform.is_amiga then
        return "/" -- AmigaDOS accepts / or volume:path
    else
        return "/"
    end
end

print("Platform:  " .. platform.os .. " (" .. platform.arch .. ")")
print("Temp Dir:  " .. get_temp_directory())
print("Separator: " .. get_path_separator())
```

---

### Example 2: Binary Serialization with Byte Order Safety
When encoding binary data packets to send across sockets or RS-232 serial links:

```lua
local platform = require("cais.platform")

local function pack_u32_big_endian(num)
    local b1 = (num >> 24) & 0xFF
    local b2 = (num >> 16) & 0xFF
    local b3 = (num >> 8)  & 0xFF
    local b4 = num & 0xFF
    return string.char(b1, b2, b3, b4)
end

local function pack_u32_native(num)
    if platform.is_big_endian then
        return pack_u32_big_endian(num)
    else
        -- Little-Endian native serialization
        local b1 = num & 0xFF
        local b2 = (num >> 8)  & 0xFF
        local b3 = (num >> 16) & 0xFF
        local b4 = (num >> 24) & 0xFF
        return string.char(b1, b2, b3, b4)
    end
end

local packet = pack_u32_native(0x10203040)
print("Serialized 4-byte payload on " .. platform.endian .. " endian system: " .. #packet .. " bytes")
```

---

## 🔬 7. Under the Hood: Endianness Detection Mechanics

Detecting CPU byte order reliably across compilers from 1985 to 2026 presents unique technical challenges:

1. **Modern Compilers (GCC 4.6+, Clang):**
   Evaluate built-in preprocessor macros `__BYTE_ORDER__` and `__ORDER_BIG_ENDIAN__`.
2. **Vintage Architecture Heuristics:**
   If built-in macros are absent, `cais` evaluates CPU architecture definitions:
   * Motorola 68000..68060 (`__m68k__`, `mc68000`) → **Big-Endian**.
   * PowerPC (`__powerpc__`, `__ppc__`) → **Big-Endian**.
   * Intel x86 / x86_64 (`__i386__`, `__x86_64__`) → **Little-Endian**.
3. **Dynamic Zero-Heap C89 Union Fallback:**
   For obscure compilers where architecture macros are unrecognized, `cais` utilizes a cached union test with zero heap allocations:
   ```c
   static const char *cais_detect_endianness(void) {
       static const char *cached = (const char *)0;
       if (cached == (const char *)0) {
           union {
               unsigned short s;
               unsigned char  c[sizeof(unsigned short)];
           } u;
           u.s = 1;
           cached = (u.c[0] == 1) ? "little" : "big";
       }
       return cached;
   }
   ```
   This guarantees that `cais.platform` **never returns an ambiguous or null endianness identifier**, regardless of how exotic the host environment is.
