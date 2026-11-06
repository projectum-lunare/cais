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

#if CAIS_IS_FREEBSD || defined(__FreeBSD__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#if defined(__FreeBSD_version) && __FreeBSD_version < 1000000
#include <sys/capability.h>
#else
#include <sys/capsicum.h>
#endif
#include <fcntl.h>
#include <unistd.h>

static int s_cached_capsicum = -1;

static int probe_capsicum(void)
{
    int val = 0;
    size_t len = sizeof(val);
    unsigned int mode = 0;

    if (s_cached_capsicum != -1) {
        return s_cached_capsicum;
    }

    if (sysctlbyname("kern.features.capsicum", &val, &len, NULL, 0) == 0) {
        s_cached_capsicum = (val != 0) ? 1 : 0;
        return s_cached_capsicum;
    }

    errno = 0;
    if (cap_getmode(&mode) == 0) {
        s_cached_capsicum = 1;
    } else {
        s_cached_capsicum = 0;
    }

    return s_cached_capsicum;
}

cais_sandbox_backend_t cais_pal_sandbox_backend(int *version_out)
{
    if (!probe_capsicum()) {
        if (version_out != NULL) {
            *version_out = 0;
        }
        return CAIS_SANDBOX_BACKEND_NONE;
    }

    if (version_out != NULL) {
#if defined(__FreeBSD_version)
        *version_out = (int)__FreeBSD_version;
#else
        *version_out = 1;
#endif
    }
    return CAIS_SANDBOX_BACKEND_CAPSICUM;
}

const char *cais_pal_sandbox_backend_name(int *version_out)
{
    cais_sandbox_backend_t b = cais_pal_sandbox_backend(version_out);
    return (b == CAIS_SANDBOX_BACKEND_CAPSICUM) ? "capsicum" : "none";
}

cais_bool_t cais_pal_sandbox_is_supported(void)
{
    return probe_capsicum() ? CAIS_TRUE : CAIS_FALSE;
}

cais_sandbox_status_t cais_pal_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap)
{
    (void)policy;

    if (!probe_capsicum()) {
        cais_set_err(err_buf, err_cap, "Capsicum capability mode is not supported by the running FreeBSD kernel");
        return CAIS_SANDBOX_UNSUPPORTED;
    }

    if (cap_enter() != 0) {
        if (errno == ENOSYS) {
            cais_set_err(err_buf, err_cap, "Capsicum capability mode is not supported by the running FreeBSD kernel");
            return CAIS_SANDBOX_UNSUPPORTED;
        }
        cais_set_err_fmt(err_buf, err_cap, "cap_enter() failed: %s", strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }

    return CAIS_SANDBOX_OK;
}

#endif /* CAIS_IS_FREEBSD || defined(__FreeBSD__) */
