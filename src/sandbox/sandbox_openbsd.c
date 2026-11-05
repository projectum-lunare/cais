/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cais/platform.h>
#include <cais/sandbox.h>

#if defined(__OpenBSD__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <unistd.h>

static int s_cached_openbsd = -1;

/*
 * Probes whether pledge() and unveil() are actually supported on the running OpenBSD system.
 * 1. pledge(NULL, NULL) tests pledge availability without modifying process promises.
 * 2. unveil(NULL, "r") returns EINVAL on kernels supporting unveil without altering state.
 *    If the syscall is not implemented in the kernel, it returns -1 with ENOSYS.
 */
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

cais_sandbox_backend_t cais_sandbox_backend(int *version_out)
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

const char *cais_sandbox_backend_name(int *version_out)
{
    cais_sandbox_backend_t b = cais_sandbox_backend(version_out);
    return (b == CAIS_SANDBOX_BACKEND_OPENBSD) ? "openbsd" : "none";
}

cais_bool_t cais_sandbox_is_supported(void)
{
    return probe_openbsd_sandbox() ? CAIS_TRUE : CAIS_FALSE;
}

cais_sandbox_status_t cais_sandbox_unveil(const char *path, int permissions, char *err_buf, size_t err_cap)
{
    char mode[8];
    int idx = 0;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!probe_openbsd_sandbox()) {
        cais_set_err(err_buf, err_cap, "OpenBSD sandbox is not supported by the running kernel");
        return CAIS_SANDBOX_UNSUPPORTED;
    }

    if (path == NULL || path[0] == '\0') {
        return CAIS_SANDBOX_OK;
    }

    if (permissions & CAIS_SANDBOX_PERM_READ) {
        mode[idx++] = 'r';
    }
    if (permissions & CAIS_SANDBOX_PERM_WRITE) {
        mode[idx++] = 'w';
        mode[idx++] = 'c';
    }
    if (permissions & CAIS_SANDBOX_PERM_EXEC) {
        mode[idx++] = 'x';
    }
    mode[idx] = '\0';

    if (unveil(path, mode) != 0) {
        if (errno == ENOSYS) {
            cais_set_err(err_buf, err_cap, "unveil is not supported by the running OpenBSD kernel");
            return CAIS_SANDBOX_UNSUPPORTED;
        }
        cais_set_err_fmt(err_buf, err_cap, "unveil(%.128s, %s) failed: %s", path, mode, strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }
    return CAIS_SANDBOX_OK;
}

cais_sandbox_status_t cais_sandbox_lock(char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!probe_openbsd_sandbox()) {
        cais_set_err(err_buf, err_cap, "OpenBSD sandbox is not supported by the running kernel");
        return CAIS_SANDBOX_UNSUPPORTED;
    }

    if (unveil(NULL, NULL) != 0) {
        if (errno == ENOSYS) {
            cais_set_err(err_buf, err_cap, "unveil is not supported by the running OpenBSD kernel");
            return CAIS_SANDBOX_UNSUPPORTED;
        }
        cais_set_err_fmt(err_buf, err_cap, "unveil(NULL, NULL) failed: %s", strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }
    return CAIS_SANDBOX_OK;
}

cais_sandbox_status_t cais_sandbox_pledge(const char *promises, char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!probe_openbsd_sandbox()) {
        cais_set_err(err_buf, err_cap, "OpenBSD sandbox is not supported by the running kernel");
        return CAIS_SANDBOX_UNSUPPORTED;
    }

    if (pledge(promises, NULL) != 0) {
        if (errno == ENOSYS) {
            cais_set_err(err_buf, err_cap, "pledge is not supported by the running OpenBSD kernel");
            return CAIS_SANDBOX_UNSUPPORTED;
        }
        cais_set_err_fmt(err_buf, err_cap, "pledge(%.128s) failed: %s", promises ? promises : "", strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }
    return CAIS_SANDBOX_OK;
}

#endif /* defined(__OpenBSD__) */
