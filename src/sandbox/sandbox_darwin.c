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

#if CAIS_IS_MACOS || defined(__APPLE__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(__has_include)
#if __has_include(<sandbox.h>)
#include <sandbox.h>
#define CAIS_HAS_DARWIN_SANDBOX_H 1
#endif
#endif

#ifndef CAIS_HAS_DARWIN_SANDBOX_H
extern int sandbox_init(const char *profile, uint64_t flags, char **errorbuf);
extern void sandbox_free_error(char *errorbuf);
#endif

cais_sandbox_backend_t cais_pal_sandbox_backend(int *version_out)
{
    if (version_out != NULL) {
        *version_out = 1;
    }
    return CAIS_SANDBOX_BACKEND_SEATBELT;
}

const char *cais_pal_sandbox_backend_name(int *version_out)
{
    if (version_out != NULL) {
        *version_out = 1;
    }
    return "seatbelt";
}

cais_bool_t cais_pal_sandbox_is_supported(void)
{
    return CAIS_TRUE;
}

/* Helper to append formatted string to growable buffer */
static void append_str(char **buf, size_t *len, size_t *cap, const char *str)
{
    size_t slen = strlen(str);
    if (*len + slen + 1 >= *cap) {
        size_t new_cap = (*cap == 0) ? 1024 : (*cap * 2);
        while (new_cap <= *len + slen + 1) {
            new_cap *= 2;
        }
        *buf = (char *)realloc(*buf, new_cap);
        *cap = new_cap;
    }
    memcpy(*buf + *len, str, slen);
    *len += slen;
    (*buf)[*len] = '\0';
}

cais_sandbox_status_t cais_pal_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap)
{
    char *profile = NULL;
    size_t len = 0;
    size_t cap = 0;
    char *err_msg = NULL;
    int rc;
    size_t path_count = 0;
    size_t i;

    /* Base SBPL Profile */
    append_str(&profile, &len, &cap, "(version 1)\n");
    append_str(&profile, &len, &cap, "(deny default)\n");
    append_str(&profile, &len, &cap, "(allow file-read-metadata)\n");
    append_str(&profile, &len, &cap, "(allow sysctl-read)\n");
    append_str(&profile, &len, &cap, "(allow mach-lookup)\n");
    append_str(&profile, &len, &cap, "(allow signal (target self))\n");

    if (policy != NULL) {
        cais_sandbox_net_t net = cais_sandbox_policy_get_network(policy);
        cais_bool_t allow_proc = cais_sandbox_policy_get_allow_child_processes(policy);
        cais_bool_t allow_dyn  = cais_sandbox_policy_get_allow_dynamic_code(policy);

        /* Network rules */
        if (net == CAIS_SANDBOX_NET_NONE) {
            append_str(&profile, &len, &cap, "(deny network*)\n");
        } else if (net == CAIS_SANDBOX_NET_LOOPBACK) {
            append_str(&profile, &len, &cap, "(allow network* (local ip \"127.0.0.1:*\"))\n");
            append_str(&profile, &len, &cap, "(allow network* (remote ip \"127.0.0.1:*\"))\n");
        } else {
            append_str(&profile, &len, &cap, "(allow network*)\n");
        }

        /* Process creation rules */
        if (!allow_proc) {
            append_str(&profile, &len, &cap, "(deny process-fork)\n");
            append_str(&profile, &len, &cap, "(deny process-exec)\n");
        } else {
            append_str(&profile, &len, &cap, "(allow process-fork)\n");
            append_str(&profile, &len, &cap, "(allow process-exec)\n");
        }

        /* Dynamic code hardening (W^X) */
        if (!allow_dyn) {
            append_str(&profile, &len, &cap, "(deny dynamic-code-generation)\n");
        }

        /* Path access rules */
        path_count = cais_sandbox_policy_path_count(policy);
        for (i = 0; i < path_count; i++) {
            int perms = 0;
            const char *path = cais_sandbox_policy_get_path(policy, i, &perms);
            char line[1024];

            if (path == NULL || path[0] == '\0') {
                continue;
            }

            if (perms & CAIS_SANDBOX_PERM_READ) {
                snprintf(line, sizeof(line), "(allow file-read* (subpath \"%.500s\"))\n", path);
                append_str(&profile, &len, &cap, line);
            }
            if (perms & CAIS_SANDBOX_PERM_WRITE) {
                snprintf(line, sizeof(line), "(allow file-write* (subpath \"%.500s\"))\n", path);
                append_str(&profile, &len, &cap, line);
            }
            if (perms & CAIS_SANDBOX_PERM_EXEC) {
                snprintf(line, sizeof(line), "(allow process-exec (literal \"%.500s\"))\n", path);
                append_str(&profile, &len, &cap, line);
            }
        }
    } else {
        /* Total lockdown */
        append_str(&profile, &len, &cap, "(deny network*)\n");
        append_str(&profile, &len, &cap, "(deny process-fork)\n");
        append_str(&profile, &len, &cap, "(deny process-exec)\n");
    }

    rc = sandbox_init(profile, 0, &err_msg);
    free(profile);

    if (rc != 0) {
        cais_set_err_fmt(err_buf, err_cap, "sandbox_init failed: %s", (err_msg != NULL) ? err_msg : "unknown error");
        if (err_msg != NULL) {
            sandbox_free_error(err_msg);
        }
        return CAIS_SANDBOX_ERROR;
    }

    return CAIS_SANDBOX_OK;
}

#endif /* CAIS_IS_MACOS || defined(__APPLE__) */
