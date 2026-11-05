/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cais/platform.h>
#include <cais/sandbox.h>

cais_sandbox_backend_t cais_sandbox_backend(int *version_out)
{
    if (version_out != NULL) {
        *version_out = 0;
    }
    return CAIS_SANDBOX_BACKEND_NONE;
}

const char *cais_sandbox_backend_name(int *version_out)
{
    cais_sandbox_backend(version_out);
    return "none";
}

cais_bool_t cais_sandbox_is_supported(void)
{
    return CAIS_FALSE;
}

cais_sandbox_status_t cais_sandbox_unveil(const char *path, int permissions, char *err_buf, size_t err_cap)
{
    (void)path;
    (void)permissions;
    cais_set_err(err_buf, err_cap, "sandboxing is not supported on this platform");
    return CAIS_SANDBOX_UNSUPPORTED;
}

cais_sandbox_status_t cais_sandbox_lock(char *err_buf, size_t err_cap)
{
    cais_set_err(err_buf, err_cap, "sandboxing is not supported on this platform");
    return CAIS_SANDBOX_UNSUPPORTED;
}

cais_sandbox_status_t cais_sandbox_pledge(const char *promises, char *err_buf, size_t err_cap)
{
    (void)promises;
    cais_set_err(err_buf, err_cap, "sandboxing is not supported on this platform");
    return CAIS_SANDBOX_UNSUPPORTED;
}
