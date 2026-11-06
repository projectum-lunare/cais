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

#if defined(__OpenBSD__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <unistd.h>

static int s_cached_openbsd = -1;

static int probe_openbsd_sandbox(void)
{
    if (s_cached_openbsd != -1) {
        return s_cached_openbsd;
    }

#if defined(OpenBSD) && OpenBSD >= 201811
    errno = 0;
    if (pledge(NULL, NULL) == 0) {
        errno = 0;
        if (unveil(NULL, "r") == -1 && errno == EINVAL) {
            s_cached_openbsd = 1;
            return 1;
        }
    }
    s_cached_openbsd = 0;
#elif defined(OpenBSD) && OpenBSD >= 201605
    errno = 0;
    if (pledge(NULL, NULL) == 0) {
        s_cached_openbsd = 1;
        return 1;
    }
    s_cached_openbsd = 0;
#else
    errno = 0;
    if (pledge(NULL, NULL) == 0) {
        s_cached_openbsd = 1;
        return 1;
    }
    s_cached_openbsd = 0;
#endif

    return s_cached_openbsd;
}

cais_sandbox_backend_t cais_pal_sandbox_backend(int *version_out)
{
    if (!probe_openbsd_sandbox()) {
        if (version_out != NULL) {
            *version_out = 0;
        }
        return CAIS_SANDBOX_BACKEND_NONE;
    }

    if (version_out != NULL) {
#if defined(OpenBSD)
        *version_out = (int)OpenBSD;
#else
        *version_out = 1;
#endif
    }
    return CAIS_SANDBOX_BACKEND_OPENBSD;
}

const char *cais_pal_sandbox_backend_name(int *version_out)
{
    cais_sandbox_backend_t b = cais_pal_sandbox_backend(version_out);
    return (b == CAIS_SANDBOX_BACKEND_OPENBSD) ? "openbsd" : "none";
}

cais_bool_t cais_pal_sandbox_is_supported(void)
{
    return probe_openbsd_sandbox() ? CAIS_TRUE : CAIS_FALSE;
}

cais_sandbox_status_t cais_pal_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap)
{
    char promises[128];
    int has_read = 0;
    int has_write = 0;
    int has_exec = 0;
    size_t path_count = 0;
    size_t i;

    if (!probe_openbsd_sandbox()) {
        cais_set_err(err_buf, err_cap, "OpenBSD sandbox (pledge/unveil) not supported on this kernel");
        return CAIS_SANDBOX_UNSUPPORTED;
    }

    if (policy != NULL) {
        path_count = cais_sandbox_policy_path_count(policy);
        for (i = 0; i < path_count; i++) {
            int perms = 0;
            const char *path = cais_sandbox_policy_get_path(policy, i, &perms);
            char mode[8];
            int idx = 0;

            if (path == NULL || path[0] == '\0') {
                continue;
            }

            if (perms & CAIS_SANDBOX_PERM_READ) {
                mode[idx++] = 'r';
                has_read = 1;
            }
            if (perms & CAIS_SANDBOX_PERM_WRITE) {
                mode[idx++] = 'w';
                mode[idx++] = 'c';
                has_write = 1;
            }
            if (perms & CAIS_SANDBOX_PERM_EXEC) {
                mode[idx++] = 'x';
                has_exec = 1;
            }
            mode[idx] = '\0';

            if (idx == 0) {
                mode[0] = 'r';
                mode[1] = '\0';
                has_read = 1;
            }

            if (unveil(path, mode) != 0) {
                cais_set_err_fmt(err_buf, err_cap, "unveil failed on '%.128s': %s", path, strerror(errno));
                return CAIS_SANDBOX_ERROR;
            }
        }
    }

    /* Seal unveil */
    if (unveil(NULL, NULL) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "unveil(NULL, NULL) seal failed: %s", strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }

    /* Synthesize pledge promises */
    strcpy(promises, "stdio");
    if (has_read) {
        strcat(promises, " rpath");
    }
    if (has_write) {
        strcat(promises, " wpath cpath");
    }
    if (has_exec) {
        strcat(promises, " exec");
    }
    if (policy != NULL) {
        if (cais_sandbox_policy_get_network(policy) != CAIS_SANDBOX_NET_NONE) {
            strcat(promises, " inet");
        }
        if (cais_sandbox_policy_get_allow_child_processes(policy)) {
            strcat(promises, " proc");
        }
    }

    if (pledge(promises, NULL) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "pledge failed with '%s': %s", promises, strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }

    return CAIS_SANDBOX_OK;
}

#endif /* defined(__OpenBSD__) */
