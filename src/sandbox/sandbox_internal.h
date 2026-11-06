/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_SANDBOX_INTERNAL_H
#define CAIS_SANDBOX_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cais/platform.h>
#include <cais/types.h>
#include <cais/sandbox.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * Policy Data Representation
 * =============================================================================
 */
typedef struct cais_sandbox_path_entry {
    char *path;
    int perms;
} cais_sandbox_path_entry_t;

struct cais_sandbox_policy {
    cais_sandbox_path_entry_t *paths;
    size_t path_count;
    size_t path_cap;
    cais_sandbox_net_t network;
    cais_bool_t allow_child_processes;
    cais_bool_t allow_dynamic_code;
};

/*
 * Policy Inspection Getters (for PAL backend drivers)
 */
size_t cais_sandbox_policy_path_count(const cais_sandbox_policy_t *policy);
const char *cais_sandbox_policy_get_path(const cais_sandbox_policy_t *policy, size_t idx, int *perms_out);
cais_sandbox_net_t cais_sandbox_policy_get_network(const cais_sandbox_policy_t *policy);
cais_bool_t cais_sandbox_policy_get_allow_child_processes(const cais_sandbox_policy_t *policy);
cais_bool_t cais_sandbox_policy_get_allow_dynamic_code(const cais_sandbox_policy_t *policy);

/*
 * =============================================================================
 * Platform Abstraction Layer (PAL) SPI Prototypes
 * =============================================================================
 */
cais_sandbox_backend_t cais_pal_sandbox_backend(int *version_out);
const char *cais_pal_sandbox_backend_name(int *version_out);
cais_bool_t cais_pal_sandbox_is_supported(void);
cais_sandbox_status_t cais_pal_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_SANDBOX_INTERNAL_H */
