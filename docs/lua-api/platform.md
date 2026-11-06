# cais.platform

The `cais.platform` module provides compile-time and runtime introspection of the host operating system, processor architecture, pointer bitness, and byte-ordering.

---

## Requiring the Module

```lua
local platform = require("cais.platform")
-- or via umbrella:
local platform = require("cais").platform
```

---

## Target Identifiers

| Field             |   Type    | Description                                                                     |
| :---------------- | :-------: | :------------------------------------------------------------------------------ |
| `platform.os`     | `string`  | Canonical operating system name (see [Supported Operating Systems](#supported-operating-systems)) |
| `platform.arch`   | `string`  | Target CPU architecture identifier (see [Supported Architectures](#supported-architectures))     |
| `platform.bits`   | `integer` | Pointer width and word size in bits (`16`, `32`, or `64`)                       |
| `platform.endian` | `string`  | Memory byte ordering (`"little"`, `"big"`, or `"unknown"`)                      |

### Supported Operating Systems

The following canonical values are detected at compile-time by the substrate and exported through `platform.os`:

| Value         | Target Operating System                               |
| :------------ | :---------------------------------------------------- |
| `"linux"`     | Linux kernel                                          |
| `"darwin"`    | Apple macOS / Darwin / iOS                            |
| `"windows"`   | Microsoft Windows NT / Win32 / Win64                  |
| `"wince"`     | Microsoft Windows CE / Pocket PC / Windows Mobile     |
| `"freebsd"`   | FreeBSD                                               |
| `"openbsd"`   | OpenBSD                                               |
| `"netbsd"`    | NetBSD                                                |
| `"dos"`       | MS-DOS / PC-DOS / FreeDOS                             |
| `"amiga"`     | Classic Commodore AmigaOS (m68k)                      |
| `"amigaos4"`  | PowerPC AmigaOS 4                                     |
| `"aros"`      | AROS Research Operating System                        |
| `"atarist"`   | Atari ST (TOS / FreeMiNT)                             |
| `"kolibri"`   | KolibriOS (MenuetOS fork)                             |
| `"haiku"`     | Haiku OS                                              |
| `"beos"`      | BeOS                                                  |
| `"plan9"`     | Plan 9 from Bell Labs                                 |
| `"os2"`       | IBM OS/2 or ArcaOS                                    |
| `"mac68k"`    | Classic Macintosh System / Mac OS (m68k)              |
| `"x68k"`      | Sharp X68000 (Human68k)                               |
| `"solaris"`   | Sun Solaris / Illumos                                 |
| `"minix"`     | Minix 3                                               |
| `"unix"`      | Generic UNIX fallback                                 |
| `"wasi"`      | WebAssembly System Interface                          |
| `"unknown"`   | Unrecognized host operating system                    |

### Supported Architectures

The following canonical CPU architecture identifiers are exported through `platform.arch`:

| Value       | Target CPU Architecture                              |
| :---------- | :--------------------------------------------------- |
| `"x86_64"`  | AMD64 / Intel 64-bit x86                             |
| `"x86"`     | Intel IA-32 (i386, i486, i586, i686)                 |
| `"arm64"`   | AArch64 / ARMv8+ 64-bit                              |
| `"arm"`     | ARM 32-bit (ARMv4T through ARMv7)                    |
| `"m68k"`    | Motorola 68000 family                                |
| `"ppc"`     | PowerPC 32-bit                                       |
| `"mips"`    | MIPS (I, II, III, IV, 32, 64)                        |
| `"alpha"`   | DEC Alpha                                            |
| `"ia64"`    | Intel Itanium 64-bit                                 |
| `"sh"`      | Hitachi / Renesas SuperH                             |
| `"riscv64"` | RISC-V 64-bit                                        |
| `"wasm32"`  | WebAssembly 32-bit                                   |
| `"unknown"` | Unrecognized CPU architecture                        |

---

## Boolean Predicate Flags

The module exports pre-computed boolean flags for fast conditional branching:

### Word Size & Bitness

* `platform.is_64bit`: True if running in a 64-bit address space.
* `platform.is_32bit`: True if running in a 32-bit address space.
* `platform.is_16bit`: True if running on a 16-bit segmented or flat architecture (e.g. MS-DOS).

### Endianness

* `platform.is_little_endian`: True for x86, ARM little-endian, etc.
* `platform.is_big_endian`: True for Motorola 68000, SPARC, PowerPC, etc.

### Operating System Family

* `platform.is_posix`: True for POSIX-compliant kernels (Linux, BSDs, macOS, Solaris).
* `platform.is_windows`: True for any Windows variant (NT, 9x, CE).
* `platform.is_windows_nt`: True specifically for modern Windows NT / Win32 / Win64.
* `platform.is_windows_ce`: True for Windows CE / Pocket PC / Windows Mobile.
* `platform.is_linux`: True for Linux kernel targets.
* `platform.is_darwin`: True for Apple macOS, iOS, Darwin.
* `platform.is_bsd`: True for FreeBSD, OpenBSD, NetBSD, or DragonFly BSD.
* `platform.is_dos`: True for MS-DOS, PC-DOS, or FreeDOS.
* `platform.is_amiga`: True for AmigaOS, AROS, or MorphOS.
* `platform.is_amiga_classic`: True specifically for m68k Commodore AmigaOS.
* `platform.is_amigaos4`: True for PowerPC AmigaOS 4.
* `platform.is_aros`: True for AROS Research Operating System.
* `platform.is_atarist`: True for Atari ST (TOS, FreeMiNT).
* `platform.is_kolibri`: True for KolibriOS.
* `platform.is_plan9`: True for Plan 9 from Bell Labs.
* `platform.is_haiku`: True for Haiku OS.
* `platform.is_beos`: True for BeOS.
* `platform.is_os2`: True for IBM OS/2 or ArcaOS.
* `platform.is_minix`: True for Minix 3.
* `platform.is_mac68k`: True for Classic Macintosh System / Mac OS (m68k).
* `platform.is_x68k`: True for Sharp X68000 (Human68k).
* `platform.is_wasi`: True for WebAssembly System Interface.

---

## Usage Example

```lua
local platform = require("cais.platform")

print(string.format("Target: %s on %s (%d-bit, %s-endian)",
    platform.os, platform.arch, platform.bits, platform.endian))

if platform.is_posix then
    print("Running in POSIX environment")
elseif platform.is_windows then
    print("Running on Microsoft Windows")
elseif platform.is_dos then
    print("Running in Real/Protected Mode DOS")
end
```
