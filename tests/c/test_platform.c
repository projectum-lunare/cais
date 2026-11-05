/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 * Mozilla Public License Version 2.0
 */

#include "test_common.h"

/*
 * ============================================================================
 * Platform Metadata & Architecture Inspection Tests
 * ============================================================================
 * Verifies that the platform abstraction layer correctly identifies the target
 * operating system, CPU architecture, pointer bitness, and memory endianness
 * configured at compile time.
 */

TEST test_platform(void)
{
    /*
     * 1. Operating System Identifier:
     *    cais_platform_os() returns a static string representing the host OS
     *    (e.g., "linux", "dos", "amigaos", "atarist", "os2", "windows").
     *    It must match the compile-time macro CAIS_OS_NAME.
     */
    ASSERT_STR_EQ(CAIS_OS_NAME, cais_platform_os());

    /*
     * 2. CPU Architecture Identifier:
     *    cais_platform_arch() returns the CPU architecture string (e.g. "x86_64",
     *    "x86", "m68k", "arm", "aarch64").
     *    It must match the compile-time macro CAIS_ARCH_NAME.
     */
    ASSERT_STR_EQ(CAIS_ARCH_NAME, cais_platform_arch());

    /*
     * 3. Machine Word Size / Bitness:
     *    cais_platform_bits() returns an integer representing pointer bitness
     *    (16 for real-mode/small models, 32 for standard 32-bit, 64 for 64-bit).
     *    It must match CAIS_BITS.
     */
    ASSERT_EQ(CAIS_BITS, cais_platform_bits());

    /*
     * 4. Byte Endianness:
     *    cais_platform_endian() returns either "little" or "big".
     *    Must never return NULL on any supported platform.
     */
    ASSERT(cais_platform_endian() != NULL);

    PASS();
}

SUITE(platform_suite)
{
    RUN_TEST(test_platform);
}
