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

cais_sandbox_backend_t cais_pal_sandbox_backend(int *version_out)
{
    if (version_out != NULL) {
        *version_out = 0;
    }
    return CAIS_SANDBOX_BACKEND_NONE;
}

const char *cais_pal_sandbox_backend_name(int *version_out)
{
    if (version_out != NULL) {
        *version_out = 0;
    }
    return "none";
}

cais_bool_t cais_pal_sandbox_is_supported(void)
{
    return CAIS_FALSE;
}

cais_sandbox_status_t cais_pal_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap)
{
    (void)policy;
    cais_set_err(err_buf, err_cap, "sandboxing is not supported on this platform");
    return CAIS_SANDBOX_UNSUPPORTED;
}
