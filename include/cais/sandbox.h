/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_SANDBOX_H
#define CAIS_SANDBOX_H

#include <cais/types.h>
#include <stddef.h>

/*
 * Sandbox backend identifiers.
 */
typedef enum {
    CAIS_SANDBOX_BACKEND_NONE     = 0,
    CAIS_SANDBOX_BACKEND_LANDLOCK = 1,
    CAIS_SANDBOX_BACKEND_OPENBSD  = 2,
    CAIS_SANDBOX_BACKEND_CAPSICUM = 3
} cais_sandbox_backend_t;

/*
 * Sandbox filesystem unveil access permissions.
 */
typedef enum {
    CAIS_SANDBOX_PERM_NONE  = 0,
    CAIS_SANDBOX_PERM_READ  = (1 << 0),
    CAIS_SANDBOX_PERM_WRITE = (1 << 1),
    CAIS_SANDBOX_PERM_EXEC  = (1 << 2)
} cais_sandbox_perm_t;

#define CAIS_SANDBOX_READ  CAIS_SANDBOX_PERM_READ
#define CAIS_SANDBOX_WRITE CAIS_SANDBOX_PERM_WRITE
#define CAIS_SANDBOX_EXEC  CAIS_SANDBOX_PERM_EXEC

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns the active sandboxing backend type enum:
 * CAIS_SANDBOX_BACKEND_NONE, CAIS_SANDBOX_BACKEND_LANDLOCK,
 * CAIS_SANDBOX_BACKEND_OPENBSD, or CAIS_SANDBOX_BACKEND_CAPSICUM.
 * If version_out is non-NULL, stores ABI version or 0.
 */
cais_sandbox_backend_t cais_sandbox_backend(int *version_out);

/*
 * Returns the name of the active sandboxing backend:
 * "landlock", "openbsd", "capsicum", or "none".
 * If version_out is non-NULL, stores ABI version or 0.
 */
const char *cais_sandbox_backend_name(int *version_out);

/**
 * @brief Checks whether kernel sandboxing is supported on the running platform.
 * @return CAIS_TRUE if a functional sandbox backend is available, CAIS_FALSE otherwise.
 */
cais_bool_t cais_sandbox_is_supported(void);

/*
 * Unveils a filesystem path with given permissions (read/write/exec).
 *
 * NOTE: Sandboxing operations are process-global.
 * Under Linux (Landlock), calls to unveil() accumulate path rules into a
 * process-wide ruleset until cais_sandbox_lock() is called to enforce them.
 * Under FreeBSD (Capsicum), path-based unveil is not supported and returns
 * CAIS_SANDBOX_UNSUPPORTED.
 *
 * Returns CAIS_SANDBOX_OK on success,
 * CAIS_SANDBOX_UNSUPPORTED if sandboxing is not supported on this platform,
 * or CAIS_SANDBOX_ERROR on failure (writing error details to err_buf if non-NULL).
 */
cais_sandbox_status_t cais_sandbox_unveil(const char *path, int permissions, char *err_buf, size_t err_cap);

/*
 * Locks the sandbox, enforcing the restrictions on the current process.
 * If called without prior unveil(), enforces an empty ruleset that prevents
 * opening any newly restricted filesystem paths.
 * Returns CAIS_SANDBOX_OK on success,
 * CAIS_SANDBOX_UNSUPPORTED if sandboxing is not supported on this platform,
 * or CAIS_SANDBOX_ERROR on failure (writing error details to err_buf if non-NULL).
 */
cais_sandbox_status_t cais_sandbox_lock(char *err_buf, size_t err_cap);

/*
 * Pledges a set of system call promises (e.g. "stdio rpath wpath").
 * On platforms supporting pledge (OpenBSD), enforces the promises.
 * On unsupported platforms, returns CAIS_SANDBOX_UNSUPPORTED (-4) and populates
 * err_buf if provided.
 * Returns CAIS_SANDBOX_OK on success, or CAIS_SANDBOX_ERROR on failure.
 */
cais_sandbox_status_t cais_sandbox_pledge(const char *promises, char *err_buf, size_t err_cap);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_SANDBOX_H */
