/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "sandbox_internal.h"

/*
 * =============================================================================
 * Introspection Forwarding to PAL
 * =============================================================================
 */

cais_sandbox_backend_t cais_sandbox_backend(int *version_out)
{
    return cais_pal_sandbox_backend(version_out);
}

const char *cais_sandbox_backend_name(int *version_out)
{
    return cais_pal_sandbox_backend_name(version_out);
}

cais_bool_t cais_sandbox_is_supported(void)
{
    return cais_pal_sandbox_is_supported();
}

/*
 * =============================================================================
 * Policy Lifecycle
 * =============================================================================
 */

cais_sandbox_policy_t *cais_sandbox_policy_create(void)
{
    cais_sandbox_policy_t *p = (cais_sandbox_policy_t *)malloc(sizeof(cais_sandbox_policy_t));
    if (p == NULL) {
        return NULL;
    }
    p->paths = NULL;
    p->path_count = 0;
    p->path_cap = 0;
    p->network = CAIS_SANDBOX_NET_DEFAULT;
    p->allow_child_processes = CAIS_TRUE;
    p->allow_dynamic_code = CAIS_TRUE;
    return p;
}

void cais_sandbox_policy_destroy(cais_sandbox_policy_t *policy)
{
    size_t i;
    if (policy == NULL) {
        return;
    }
    if (policy->paths != NULL) {
        for (i = 0; i < policy->path_count; i++) {
            if (policy->paths[i].path != NULL) {
                free(policy->paths[i].path);
            }
        }
        free(policy->paths);
    }
    free(policy);
}

cais_status_t cais_sandbox_policy_allow_path(cais_sandbox_policy_t *policy, const char *path, int perms)
{
    char *copy;
    if (policy == NULL || path == NULL || path[0] == '\0') {
        return CAIS_ERROR;
    }

    if (policy->path_count >= policy->path_cap) {
        size_t new_cap = (policy->path_cap == 0) ? 8 : (policy->path_cap * 2);
        cais_sandbox_path_entry_t *new_entries = (cais_sandbox_path_entry_t *)realloc(
            policy->paths, new_cap * sizeof(cais_sandbox_path_entry_t)
        );
        if (new_entries == NULL) {
            return CAIS_ERROR;
        }
        policy->paths = new_entries;
        policy->path_cap = new_cap;
    }

    copy = cais_strdup(path);
    if (copy == NULL) {
        return CAIS_ERROR;
    }

    policy->paths[policy->path_count].path = copy;
    policy->paths[policy->path_count].perms = perms;
    policy->path_count++;
    return CAIS_OK;
}

cais_status_t cais_sandbox_policy_set_network(cais_sandbox_policy_t *policy, cais_sandbox_net_t net)
{
    if (policy == NULL) {
        return CAIS_ERROR;
    }
    policy->network = net;
    return CAIS_OK;
}

cais_status_t cais_sandbox_policy_set_allow_child_processes(cais_sandbox_policy_t *policy, cais_bool_t allow)
{
    if (policy == NULL) {
        return CAIS_ERROR;
    }
    policy->allow_child_processes = allow;
    return CAIS_OK;
}

cais_status_t cais_sandbox_policy_set_allow_dynamic_code(cais_sandbox_policy_t *policy, cais_bool_t allow)
{
    if (policy == NULL) {
        return CAIS_ERROR;
    }
    policy->allow_dynamic_code = allow;
    return CAIS_OK;
}

/*
 * =============================================================================
 * Policy Getters (for PAL drivers)
 * =============================================================================
 */

size_t cais_sandbox_policy_path_count(const cais_sandbox_policy_t *policy)
{
    return (policy != NULL) ? policy->path_count : 0;
}

const char *cais_sandbox_policy_get_path(const cais_sandbox_policy_t *policy, size_t idx, int *perms_out)
{
    if (policy == NULL || idx >= policy->path_count) {
        if (perms_out != NULL) *perms_out = 0;
        return NULL;
    }
    if (perms_out != NULL) {
        *perms_out = policy->paths[idx].perms;
    }
    return policy->paths[idx].path;
}

cais_sandbox_net_t cais_sandbox_policy_get_network(const cais_sandbox_policy_t *policy)
{
    return (policy != NULL) ? policy->network : CAIS_SANDBOX_NET_DEFAULT;
}

cais_bool_t cais_sandbox_policy_get_allow_child_processes(const cais_sandbox_policy_t *policy)
{
    return (policy != NULL) ? policy->allow_child_processes : CAIS_TRUE;
}

cais_bool_t cais_sandbox_policy_get_allow_dynamic_code(const cais_sandbox_policy_t *policy)
{
    return (policy != NULL) ? policy->allow_dynamic_code : CAIS_TRUE;
}

/*
 * =============================================================================
 * Enforcement
 * =============================================================================
 */

cais_sandbox_status_t cais_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }
    return cais_pal_sandbox_apply(policy, err_buf, err_cap);
}
