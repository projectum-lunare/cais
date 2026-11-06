/*
 * cais - Communication Abstraction & I/O Substrate
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
 * =============================================================================
 * Sandbox Backend Identifiers
 * =============================================================================
 */
typedef enum {
    CAIS_SANDBOX_BACKEND_NONE     = 0,
    CAIS_SANDBOX_BACKEND_LANDLOCK = 1, /* Linux Landlock LSM */
    CAIS_SANDBOX_BACKEND_OPENBSD  = 2, /* OpenBSD pledge & unveil */
    CAIS_SANDBOX_BACKEND_CAPSICUM = 3, /* FreeBSD Capsicum capability mode */
    CAIS_SANDBOX_BACKEND_SEATBELT = 4, /* Apple Darwin / macOS Seatbelt (sandbox_init) */
    CAIS_SANDBOX_BACKEND_WIN32    = 5  /* Windows NT Process Mitigations & Token Lockdown */
} cais_sandbox_backend_t;

/*
 * =============================================================================
 * Filesystem Permission Flags for Allowed Paths
 * =============================================================================
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

/*
 * =============================================================================
 * Network Access Policy Level
 * =============================================================================
 */
typedef enum {
    CAIS_SANDBOX_NET_DEFAULT  = 0, /* Unmodified / allow network */
    CAIS_SANDBOX_NET_NONE     = 1, /* Completely deny socket & network operations */
    CAIS_SANDBOX_NET_LOOPBACK = 2, /* Allow only local loopback communication */
    CAIS_SANDBOX_NET_ALL      = 3  /* Explicitly allow all network access */
} cais_sandbox_net_t;

/*
 * Opaque declaration of the declarative sandbox policy handle.
 */
typedef struct cais_sandbox_policy cais_sandbox_policy_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * Introspection & Platform Capability
 * =============================================================================
 */

/**
 * @brief Returns the active sandboxing backend type enum.
 * @param version_out Optional pointer to store backend ABI or OS version.
 */
cais_sandbox_backend_t cais_sandbox_backend(int *version_out);

/**
 * @brief Returns the descriptive name of the active sandboxing backend:
 *        "landlock", "seatbelt", "win32", "openbsd", "capsicum", or "none".
 * @param version_out Optional pointer to store backend ABI or OS version.
 */
const char *cais_sandbox_backend_name(int *version_out);

/**
 * @brief Checks whether kernel sandboxing is supported on the running platform.
 * @return CAIS_TRUE if a functional sandbox backend is available, CAIS_FALSE otherwise.
 */
cais_bool_t cais_sandbox_is_supported(void);

/*
 * =============================================================================
 * Declarative Policy Lifecycle & Configuration
 * =============================================================================
 */

/**
 * @brief Allocates and initializes a new empty declarative sandbox policy.
 * @return Pointer to new policy handle, or NULL on memory allocation failure.
 */
cais_sandbox_policy_t *cais_sandbox_policy_create(void);

/**
 * @brief Releases all resources associated with a sandbox policy handle.
 * @param policy Policy handle to destroy (safe to pass NULL).
 */
void cais_sandbox_policy_destroy(cais_sandbox_policy_t *policy);

/**
 * @brief Adds a filesystem path rule to the policy.
 * @param policy Active policy handle.
 * @param path Filesystem path (directory or file) to allow.
 * @param perms Bitmask combination of CAIS_SANDBOX_PERM_READ, WRITE, EXEC.
 * @return CAIS_OK on success, or CAIS_ERROR on invalid argument/allocation failure.
 */
cais_status_t cais_sandbox_policy_allow_path(cais_sandbox_policy_t *policy, const char *path, int perms);

/**
 * @brief Configures network access policy.
 * @param policy Active policy handle.
 * @param net Network access level (CAIS_SANDBOX_NET_DEFAULT, NONE, LOOPBACK, ALL).
 * @return CAIS_OK on success.
 */
cais_status_t cais_sandbox_policy_set_network(cais_sandbox_policy_t *policy, cais_sandbox_net_t net);

/**
 * @brief Configures whether creating child processes (fork/exec/CreateProcess) is allowed.
 * @param policy Active policy handle.
 * @param allow CAIS_TRUE to allow child processes, CAIS_FALSE to prohibit.
 * @return CAIS_OK on success.
 */
cais_status_t cais_sandbox_policy_set_allow_child_processes(cais_sandbox_policy_t *policy, cais_bool_t allow);

/**
 * @brief Configures whether dynamic code generation (W^X / RWX executable memory) is allowed.
 * @param policy Active policy handle.
 * @param allow CAIS_TRUE to allow, CAIS_FALSE to prohibit (W^X memory hardening).
 * @return CAIS_OK on success.
 */
cais_status_t cais_sandbox_policy_set_allow_dynamic_code(cais_sandbox_policy_t *policy, cais_bool_t allow);

/*
 * =============================================================================
 * Enforcement
 * =============================================================================
 */

/**
 * @brief Irrevocably enforces the sandbox policy on the current process and descendants.
 *
 * If policy is NULL, enforces a strict default lockdown (no filesystem access,
 * dropping privileges where supported by the operating system).
 *
 * @param policy The policy configuration to enforce (or NULL for complete lockdown).
 * @param err_buf Buffer to receive human-readable error description on failure.
 * @param err_cap Capacity of err_buf in bytes.
 * @return CAIS_SANDBOX_OK on success,
 *         CAIS_SANDBOX_UNSUPPORTED if sandboxing is not supported on this platform,
 *         or CAIS_SANDBOX_ERROR on failure.
 */
cais_sandbox_status_t cais_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_SANDBOX_H */
