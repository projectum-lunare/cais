/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_PLATFORM_H
#define CAIS_PLATFORM_H

#include <cais/types.h>

/* =============================================================================
 * Compiler Identification
 * ============================================================================= */
#if defined(__clang__)
#define CAIS_CC_CLANG 1
#define CAIS_CC_NAME  "clang"
#elif defined(__GNUC__)
#define CAIS_CC_GCC  1
#define CAIS_CC_NAME "gcc"
#elif defined(_MSC_VER)
#define CAIS_CC_MSVC 1
#define CAIS_CC_NAME "msvc"
#elif defined(__WATCOMC__)
#define CAIS_CC_WATCOM 1
#define CAIS_CC_NAME   "watcom"
#elif defined(__BORLANDC__)
#define CAIS_CC_BORLAND 1
#define CAIS_CC_NAME    "borland"
#elif defined(__VBCC__)
#define CAIS_CC_VBCC 1
#define CAIS_CC_NAME "vbcc"
#elif defined(__SASC) || defined(_AMIGA_SASC)
#define CAIS_CC_SAS  1
#define CAIS_CC_NAME "sasc"
#elif defined(__TINYC__)
#define CAIS_CC_TCC  1
#define CAIS_CC_NAME "tcc"
#else
#define CAIS_CC_UNKNOWN 1
#define CAIS_CC_NAME    "unknown"
#endif

/* =============================================================================
 * Portability & C89 Vintage Compiler Compatibility Helpers
 * ============================================================================= */
#ifndef CAIS_UNUSED
#define CAIS_UNUSED(x) ((void)(x))
#endif

#ifndef CAIS_INLINE
#if defined(__cplusplus)
#define CAIS_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define CAIS_INLINE static __inline__
#elif defined(_MSC_VER) || defined(__WATCOMC__)
#define CAIS_INLINE static __inline
#else
#define CAIS_INLINE static
#endif
#endif

#ifndef CAIS_UNUSED_FN
#if defined(__GNUC__) || defined(__clang__)
#define CAIS_UNUSED_FN __attribute__((unused))
#else
#define CAIS_UNUSED_FN
#endif
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * Portable safe strdup for environments without standard POSIX strdup.
 * Allocates a duplicate of s using malloc and memcpy. Returns NULL if s is NULL
 * or if memory allocation fails.
 */
CAIS_UNUSED_FN CAIS_INLINE char *cais_strdup(const char *s)
{
    size_t len;
    char *copy;
    if (s == NULL)
        return NULL;
    len = strlen(s);
    copy = (char *)malloc(len + 1);
    if (copy != NULL) {
        memcpy(copy, s, len + 1);
    }
    return copy;
}

#ifndef safe_strdup
#define safe_strdup cais_strdup
#endif

/* =============================================================================
 * Operating System Identification & Normalization (CAIS_IS_*)
 * ============================================================================= */
#if defined(__AROS__)
#define CAIS_IS_AROS  1
#define CAIS_IS_AMIGA 1
#define CAIS_OS_NAME  "aros"

#elif defined(__amigaos4__)
#define CAIS_IS_AMIGAOS4 1
#define CAIS_IS_AMIGA    1
#define CAIS_OS_NAME     "amigaos4"

#elif defined(__AMIGA__) || defined(AMIGA) || defined(_AMIGA) || defined(__amigaos__)
#define CAIS_IS_AMIGA_CLASSIC 1
#define CAIS_IS_AMIGA         1
#define CAIS_OS_NAME          "amiga"

#elif defined(__KOLIBRIOS__) || defined(__MENUETOS__)
#define CAIS_IS_KOLIBRI 1
#define CAIS_OS_NAME    "kolibri"

#elif defined(_PLAN9_SOURCE) || defined(__plan9__) || defined(_PLAN9)
#define CAIS_IS_PLAN9 1
#define CAIS_OS_NAME  "plan9"

#elif defined(_WIN32_WCE) || defined(UNDER_CE)
#define CAIS_IS_WINDOWS_CE 1
#define CAIS_IS_WINDOWS    1
#define CAIS_OS_NAME       "wince"

#elif defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#define CAIS_IS_WINDOWS_NT 1
#define CAIS_IS_WINDOWS    1
#define CAIS_OS_NAME       "windows"

#elif defined(__MSDOS__) || defined(MSDOS) || defined(_MSDOS) || defined(__DOS__)
#define CAIS_IS_DOS  1
#define CAIS_OS_NAME "dos"

#elif defined(__OS2__) || defined(_OS2)
#define CAIS_IS_OS2  1
#define CAIS_OS_NAME "os2"

#elif defined(__HAIKU__)
#define CAIS_IS_HAIKU 1
#define CAIS_IS_POSIX 1
#define CAIS_OS_NAME  "haiku"

#elif defined(__BEOS__)
#define CAIS_IS_BEOS  1
#define CAIS_IS_POSIX 1
#define CAIS_OS_NAME  "beos"

#elif defined(__ATARIST__) || defined(__atarist__) || defined(__atarist) || defined(atarist) ||    \
    defined(__TOS__) || defined(__MINT__)
#define CAIS_IS_ATARIST 1
#define CAIS_OS_NAME    "atarist"

#elif defined(macintosh) || defined(__MACOS__)
#define CAIS_IS_MAC68K 1
#define CAIS_OS_NAME   "mac68k"

#elif defined(_HUMAN68K__) || defined(__human68k__)
#define CAIS_IS_X68K 1
#define CAIS_OS_NAME "x68k"

#elif defined(__wasi__)
#define CAIS_IS_WASI 1
#define CAIS_OS_NAME "wasi"

#elif defined(__linux__)
#define CAIS_IS_LINUX 1
#define CAIS_IS_POSIX 1
#define CAIS_OS_NAME  "linux"

#elif defined(__APPLE__) && defined(__MACH__)
#define CAIS_IS_DARWIN 1
#define CAIS_IS_POSIX  1
#define CAIS_OS_NAME   "darwin"

#elif defined(__FreeBSD__)
#define CAIS_IS_FREEBSD 1
#define CAIS_IS_BSD     1
#define CAIS_IS_POSIX   1
#define CAIS_OS_NAME    "freebsd"

#elif defined(__OpenBSD__)
#define CAIS_IS_OPENBSD 1
#define CAIS_IS_BSD     1
#define CAIS_IS_POSIX   1
#define CAIS_OS_NAME    "openbsd"

#elif defined(__NetBSD__)
#define CAIS_IS_NETBSD 1
#define CAIS_IS_BSD    1
#define CAIS_IS_POSIX  1
#define CAIS_OS_NAME   "netbsd"

#elif defined(__minix) || defined(__minix__) || defined(_MINIX)
#define CAIS_IS_MINIX 1
#define CAIS_IS_POSIX 1
#define CAIS_OS_NAME  "minix"

#elif defined(__sun) && defined(__SVR4)
#define CAIS_IS_SOLARIS 1
#define CAIS_IS_POSIX   1
#define CAIS_OS_NAME    "solaris"

#elif defined(__unix__) || defined(__unix)
#define CAIS_IS_GENERIC_UNIX 1
#define CAIS_IS_POSIX        1
#define CAIS_OS_NAME         "unix"

#else
#define CAIS_IS_UNKNOWN 1
#define CAIS_OS_NAME    "unknown"
#endif

/* =============================================================================
 * Fallback Defaults for Boolean Platform Predicates (0 when inactive)
 * ============================================================================= */
#ifndef CAIS_IS_POSIX
#define CAIS_IS_POSIX 0
#endif
#ifndef CAIS_IS_WINDOWS
#define CAIS_IS_WINDOWS 0
#endif
#ifndef CAIS_IS_WINDOWS_NT
#define CAIS_IS_WINDOWS_NT 0
#endif
#ifndef CAIS_IS_WINDOWS_CE
#define CAIS_IS_WINDOWS_CE 0
#endif
#ifndef CAIS_IS_AMIGA
#define CAIS_IS_AMIGA 0
#endif
#ifndef CAIS_IS_AMIGA_CLASSIC
#define CAIS_IS_AMIGA_CLASSIC 0
#endif
#ifndef CAIS_IS_AROS
#define CAIS_IS_AROS 0
#endif
#ifndef CAIS_IS_AMIGAOS4
#define CAIS_IS_AMIGAOS4 0
#endif
#ifndef CAIS_IS_LINUX
#define CAIS_IS_LINUX 0
#endif
#ifndef CAIS_IS_DARWIN
#define CAIS_IS_DARWIN 0
#endif
#ifndef CAIS_IS_BSD
#define CAIS_IS_BSD 0
#endif
#ifndef CAIS_IS_FREEBSD
#define CAIS_IS_FREEBSD 0
#endif
#ifndef CAIS_IS_OPENBSD
#define CAIS_IS_OPENBSD 0
#endif
#ifndef CAIS_IS_NETBSD
#define CAIS_IS_NETBSD 0
#endif
#ifndef CAIS_IS_MINIX
#define CAIS_IS_MINIX 0
#endif
#ifndef CAIS_IS_SOLARIS
#define CAIS_IS_SOLARIS 0
#endif
#ifndef CAIS_IS_GENERIC_UNIX
#define CAIS_IS_GENERIC_UNIX 0
#endif
#ifndef CAIS_IS_HAIKU
#define CAIS_IS_HAIKU 0
#endif
#ifndef CAIS_IS_BEOS
#define CAIS_IS_BEOS 0
#endif
#ifndef CAIS_IS_PLAN9
#define CAIS_IS_PLAN9 0
#endif
#ifndef CAIS_IS_KOLIBRI
#define CAIS_IS_KOLIBRI 0
#endif
#ifndef CAIS_IS_DOS
#define CAIS_IS_DOS 0
#endif
#ifndef CAIS_IS_OS2
#define CAIS_IS_OS2 0
#endif
#ifndef CAIS_IS_ATARIST
#define CAIS_IS_ATARIST 0
#endif
#ifndef CAIS_IS_MAC68K
#define CAIS_IS_MAC68K 0
#endif
#ifndef CAIS_IS_X68K
#define CAIS_IS_X68K 0
#endif
#ifndef CAIS_IS_WASI
#define CAIS_IS_WASI 0
#endif
#ifndef CAIS_IS_UNKNOWN
#define CAIS_IS_UNKNOWN 0
#endif

#if CAIS_IS_POSIX || CAIS_IS_AMIGA || CAIS_IS_WINDOWS || CAIS_IS_KOLIBRI ||            \
    CAIS_IS_PLAN9 || CAIS_IS_DOS
#define CAIS_OS_HAS_NATIVE_DRIVERS 1
#endif

/* =============================================================================
 * Architecture (CPU Arch) Identification & Normalization
 * ============================================================================= */
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#define CAIS_ARCH_X86_64 1
#define CAIS_ARCH_NAME   "x86_64"

#elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
#define CAIS_ARCH_X86  1
#define CAIS_ARCH_NAME "x86"

#elif defined(__aarch64__) || defined(_M_ARM64)
#define CAIS_ARCH_ARM64 1
#define CAIS_ARCH_NAME  "arm64"

#elif defined(__arm__) || defined(_M_ARM)
#define CAIS_ARCH_ARM  1
#define CAIS_ARCH_NAME "arm"

#elif defined(__m68k__) || defined(mc68000) || defined(_M_M68K)
#define CAIS_ARCH_M68K 1
#define CAIS_ARCH_NAME "m68k"

#elif defined(__powerpc__) || defined(__ppc__) || defined(_M_PPC)
#define CAIS_ARCH_PPC  1
#define CAIS_ARCH_NAME "ppc"

#elif defined(__mips__) || defined(_M_MRX000)
#define CAIS_ARCH_MIPS 1
#define CAIS_ARCH_NAME "mips"

#elif defined(__alpha__) || defined(_M_ALPHA)
#define CAIS_ARCH_ALPHA 1
#define CAIS_ARCH_NAME  "alpha"

#elif defined(__ia64__) || defined(_M_IA64)
#define CAIS_ARCH_IA64 1
#define CAIS_ARCH_NAME "ia64"

#elif defined(__sh__) || defined(_M_SH)
#define CAIS_ARCH_SH   1
#define CAIS_ARCH_NAME "sh"

#elif defined(__riscv) && (__riscv_xlen == 64)
#define CAIS_ARCH_RISCV64 1
#define CAIS_ARCH_NAME    "riscv64"

#elif defined(__wasm32__)
#define CAIS_ARCH_WASM32 1
#define CAIS_ARCH_NAME   "wasm32"

#else
#define CAIS_ARCH_UNKNOWN 1
#define CAIS_ARCH_NAME    "unknown"
#endif

/* =============================================================================
 * Word Size / Bitness Normalization
 * ============================================================================= */
#if defined(CAIS_ARCH_X86_64) || defined(CAIS_ARCH_ARM64) || defined(CAIS_ARCH_ALPHA) ||  \
    defined(CAIS_ARCH_IA64) || defined(CAIS_ARCH_RISCV64) || defined(__LP64__) ||            \
    defined(_LP64) || (defined(__WORDSIZE) && (__WORDSIZE == 64))
#define CAIS_BITS 64
#elif CAIS_IS_DOS && !defined(__DJGPP__) && !defined(__WATCOMC__)
#define CAIS_BITS 16
#else
#define CAIS_BITS 32
#endif

#define CAIS_IS_64BIT (CAIS_BITS == 64)
#define CAIS_IS_32BIT (CAIS_BITS == 32)
#define CAIS_IS_16BIT (CAIS_BITS == 16)

/* =============================================================================
 * Endianness / Byte Order Determination
 * ============================================================================= */
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) &&                                    \
    (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define CAIS_ENDIAN_BIG 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&                               \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define CAIS_ENDIAN_LITTLE 1
#elif defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN)
#define CAIS_ENDIAN_BIG 1
#elif defined(__LITTLE_ENDIAN__) || defined(_LITTLE_ENDIAN)
#define CAIS_ENDIAN_LITTLE 1
#elif defined(CAIS_ARCH_M68K) || defined(CAIS_ARCH_PPC)
#define CAIS_ENDIAN_BIG 1
#elif defined(CAIS_ARCH_X86) || defined(CAIS_ARCH_X86_64) || defined(CAIS_ARCH_ALPHA) ||  \
    defined(CAIS_ARCH_IA64) || defined(CAIS_ARCH_WASM32)
#define CAIS_ENDIAN_LITTLE 1
#endif

#if defined(CAIS_ENDIAN_LITTLE)
#define CAIS_IS_LITTLE_ENDIAN  1
#define CAIS_IS_BIG_ENDIAN     0
#define CAIS_GET_ENDIAN_NAME() "little"

#elif defined(CAIS_ENDIAN_BIG)
#define CAIS_IS_LITTLE_ENDIAN  0
#define CAIS_IS_BIG_ENDIAN     1
#define CAIS_GET_ENDIAN_NAME() "big"

#else
/*
 * Dynamic C89 union fallback for vintage compilers without byte-order macros.
 */
static const char *cais_detect_endianness(void)
{
    static const char *cached = (const char *)0;
    if (cached == (const char *)0) {
        union {
            unsigned short s;
            unsigned char c[sizeof(unsigned short)];
        } u;
        u.s = 1;
        cached = (u.c[0] == 1) ? "little" : "big";
    }
    return cached;
}
#define CAIS_GET_ENDIAN_NAME() cais_detect_endianness()
#define CAIS_IS_LITTLE_ENDIAN  (cais_detect_endianness()[0] == 'l')
#define CAIS_IS_BIG_ENDIAN     (cais_detect_endianness()[0] == 'b')
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns host OS, architecture, bits and endianness at runtime.
 */
const char *cais_platform_os(void);
const char *cais_platform_arch(void);
int cais_platform_bits(void);
const char *cais_platform_endian(void);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_PLATFORM_H */
