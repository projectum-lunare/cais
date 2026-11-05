/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_TYPES_H
#define CAIS_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * Canonical Boolean Type & Values
 * =============================================================================
 * Strict C89/C90 compatible boolean type with seamless C99 <stdbool.h> and C++
 * interoperability. Guarantees 0 is CAIS_FALSE and 1 is CAIS_TRUE across all
 * architectures and compilers.
 */
#if defined(__cplusplus)
    typedef bool cais_bool_t;
    #ifndef CAIS_FALSE
    #define CAIS_FALSE false
    #define CAIS_TRUE  true
    #endif
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
    #include <stdbool.h>
    typedef bool cais_bool_t;
    #ifndef CAIS_FALSE
    #define CAIS_FALSE false
    #define CAIS_TRUE  true
    #endif
#else
    typedef enum cais_bool {
        CAIS_FALSE = 0,
        CAIS_TRUE  = 1
    } cais_bool_t;
    #ifndef CAIS_FALSE
    #define CAIS_FALSE CAIS_FALSE
    #define CAIS_TRUE  CAIS_TRUE
    #endif
#endif

/*
 * =============================================================================
 * Canonical Hardware Logic Level / Signal Line State Enum
 * =============================================================================
 * Expresses digital logic levels and physical signal line states across
 * serial, parallel, and GPIO interfaces without ambiguous tri-state integers.
 */
typedef enum cais_line_state {
    CAIS_LINE_ERROR = -1, /* Signal query failed or unsupported on hardware */
    CAIS_LINE_LOW   =  0, /* Deasserted / inactive / logical low (0) */
    CAIS_LINE_HIGH  =  1  /* Asserted / active / logical high (1) */
} cais_line_state_t;

#define CAIS_LINE_OFF      CAIS_LINE_LOW
#define CAIS_LINE_ON       CAIS_LINE_HIGH
#define CAIS_LINE_INACTIVE CAIS_LINE_LOW
#define CAIS_LINE_ACTIVE   CAIS_LINE_HIGH

/*
 * =============================================================================
 * Canonical I/O and System Status Enum
 * =============================================================================
 * Unifies return codes across all cais subsystems (socket, serial, parallel,
 * sandbox, filesystem).
 */
typedef enum cais_status {
    CAIS_OK          =  0, /* Operation succeeded (full or partial data transferred) */
    CAIS_ERROR       = -1, /* Fatal I/O, descriptor, or operating system error */
    CAIS_AGAIN       = -2, /* Non-blocking call would block, or timeout expired */
    CAIS_EOF         = -3, /* End of stream / peer connection closed gracefully */
    CAIS_UNSUPPORTED = -4  /* Operation or hardware mode not supported on platform */
} cais_status_t;

/* Backward-compatible subsystem aliases */
typedef cais_status_t cais_socket_status_t;
typedef cais_status_t cais_serial_status_t;
typedef cais_status_t cais_parallel_status_t;
typedef cais_status_t cais_sandbox_status_t;
typedef cais_status_t cais_fs_status_t;

#define CAIS_SOCK_OK            CAIS_OK
#define CAIS_SOCK_ERROR         CAIS_ERROR
#define CAIS_SOCK_AGAIN         CAIS_AGAIN
#define CAIS_SOCK_CLOSED        CAIS_EOF

#define CAIS_SERIAL_OK          CAIS_OK
#define CAIS_SERIAL_ERROR       CAIS_ERROR
#define CAIS_SERIAL_UNSUPPORTED CAIS_UNSUPPORTED

#define CAIS_PARALLEL_OK          CAIS_OK
#define CAIS_PARALLEL_ERROR       CAIS_ERROR
#define CAIS_PARALLEL_UNSUPPORTED CAIS_UNSUPPORTED

#define CAIS_SANDBOX_OK          CAIS_OK
#define CAIS_SANDBOX_ERROR       CAIS_ERROR
#define CAIS_SANDBOX_UNSUPPORTED CAIS_UNSUPPORTED

#define CAIS_FS_OK          CAIS_OK
#define CAIS_FS_ERROR       CAIS_ERROR
#define CAIS_FS_UNSUPPORTED CAIS_UNSUPPORTED
#define CAIS_FS_EOF         CAIS_EOF

/*
 * Returns a static descriptive error string corresponding to a cais_status_t code.
 * Returned strings reside in static read-only storage and must never be freed.
 */
const char *cais_strerror(cais_status_t status);

/*
 * Safe C89 error buffer formatting helpers.
 * Formats message into err_buf (if err_buf != NULL && err_cap > 0).
 * Guarantees NUL termination within err_cap without heap allocation.
 */
void cais_set_err(char *err_buf, size_t err_cap, const char *msg);
void cais_set_err_fmt(char *err_buf, size_t err_cap, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_TYPES_H */
