/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 * Mozilla Public License Version 2.0
 */

#include "test_common.h"

/*
 * ============================================================================
 * Cais C Unit Test Suite Entry Point
 * ============================================================================
 * This executable orchestrates and executes all modular test suites for the
 * Cais native C library using the Greatest testing framework.
 *
 * Each subsystem test suite is defined in its own compilation unit (.c) and
 * declared here via SUITE_EXTERN():
 * - platform_suite:   Host OS, architecture, word bitness, and endianness.
 * - filesystem_suite: Portable file and directory existence queries.
 * - sandbox_suite:    Landlock/Capsicum/Pledge isolation and permission controls.
 * - serial_suite:     UART/RS-232 serial driver, PTY loopback, baud rates.
 * - parallel_suite:   IEEE 1284 / SPP parallel port registers and status pins.
 * - socket_suite:     TCP/UDP networking, 64KB streaming, non-blocking I/O & polling.
 */

/* External test suite declarations */
#ifndef CAIS_NO_PLATFORM
SUITE_EXTERN(platform_suite);
#endif
#ifndef CAIS_NO_FILESYSTEM
SUITE_EXTERN(filesystem_suite);
#endif
#ifndef CAIS_NO_SANDBOX
SUITE_EXTERN(sandbox_suite);
#endif
#ifndef CAIS_NO_SERIAL
SUITE_EXTERN(serial_suite);
#endif
#ifndef CAIS_NO_PARALLEL
SUITE_EXTERN(parallel_suite);
#endif
#ifndef CAIS_NO_SOCKET
SUITE_EXTERN(socket_suite);
#endif
#ifndef CAIS_NO_NOISE
SUITE_EXTERN(noise_suite);
#endif
#ifndef CAIS_NO_TLS
SUITE_EXTERN(tls_suite);
#endif

/* Greatest framework global definition macro (must appear only once across the binary) */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    /* Initialize greatest framework with command-line flags (e.g. -v for verbose) */
    GREATEST_MAIN_BEGIN();

    /* Sequentially execute modular test suites */
#ifndef CAIS_NO_PLATFORM
    RUN_SUITE(platform_suite);
#endif
#ifndef CAIS_NO_FILESYSTEM
    RUN_SUITE(filesystem_suite);
#endif
#ifndef CAIS_NO_SANDBOX
    RUN_SUITE(sandbox_suite);
#endif
#ifndef CAIS_NO_SERIAL
    RUN_SUITE(serial_suite);
#endif
#ifndef CAIS_NO_PARALLEL
    RUN_SUITE(parallel_suite);
#endif
#ifndef CAIS_NO_SOCKET
    RUN_SUITE(socket_suite);
#endif
#ifndef CAIS_NO_NOISE
    RUN_SUITE(noise_suite);
#endif
#ifndef CAIS_NO_TLS
    RUN_SUITE(tls_suite);
#endif

    /* Print test summary (total tests, passed, failed, skipped, assertions) and exit */
    GREATEST_MAIN_END();
}
