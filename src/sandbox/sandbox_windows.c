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

#if CAIS_IS_WINDOWS || defined(_WIN32)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define CAIS_WIN_PROCESS_DYNAMIC_CODE_POLICY        2
#define CAIS_WIN_PROCESS_STRICT_HANDLE_CHECK_POLICY 6
#define CAIS_WIN_PROCESS_SYSTEM_CALL_DISABLE_POLICY 7
#define CAIS_WIN_PROCESS_CHILD_PROCESS_POLICY       13

typedef struct _CAIS_WIN_CHILD_PROCESS_POLICY {
    DWORD Flags;
} CAIS_WIN_CHILD_PROCESS_POLICY;

typedef struct _CAIS_WIN_DYNAMIC_CODE_POLICY {
    DWORD Flags;
} CAIS_WIN_DYNAMIC_CODE_POLICY;

typedef struct _CAIS_WIN_STRICT_HANDLE_CHECK_POLICY {
    DWORD Flags;
} CAIS_WIN_STRICT_HANDLE_CHECK_POLICY;

typedef BOOL (WINAPI *pfn_SetProcessMitigationPolicy_t)(int MitigationPolicy, PVOID lpBuffer, SIZE_T dwLength);

static pfn_SetProcessMitigationPolicy_t s_pfn_SetProcessMitigationPolicy = NULL;
static int s_probed_mitigation = 0;

static void probe_windows_mitigations(void)
{
    if (!s_probed_mitigation) {
        HMODULE hKernel = GetModuleHandleA("kernel32.dll");
        if (hKernel != NULL) {
            s_pfn_SetProcessMitigationPolicy = (pfn_SetProcessMitigationPolicy_t)GetProcAddress(
                hKernel, "SetProcessMitigationPolicy"
            );
        }
        s_probed_mitigation = 1;
    }
}

cais_sandbox_backend_t cais_pal_sandbox_backend(int *version_out)
{
    probe_windows_mitigations();
    if (s_pfn_SetProcessMitigationPolicy != NULL) {
        if (version_out != NULL) {
            *version_out = 1;
        }
        return CAIS_SANDBOX_BACKEND_WIN32;
    }
    if (version_out != NULL) {
        *version_out = 0;
    }
    return CAIS_SANDBOX_BACKEND_NONE;
}

const char *cais_pal_sandbox_backend_name(int *version_out)
{
    cais_sandbox_backend_t b = cais_pal_sandbox_backend(version_out);
    return (b == CAIS_SANDBOX_BACKEND_WIN32) ? "win32" : "none";
}

cais_bool_t cais_pal_sandbox_is_supported(void)
{
    int version = 0;
    return (cais_pal_sandbox_backend(&version) != CAIS_SANDBOX_BACKEND_NONE) ? CAIS_TRUE : CAIS_FALSE;
}

cais_sandbox_status_t cais_pal_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap)
{
    probe_windows_mitigations();
    if (s_pfn_SetProcessMitigationPolicy == NULL) {
        cais_set_err(err_buf, err_cap, "SetProcessMitigationPolicy is not available on this version of Windows");
        return CAIS_SANDBOX_UNSUPPORTED;
    }

    if (policy != NULL) {
        cais_bool_t allow_proc = cais_sandbox_policy_get_allow_child_processes(policy);
        cais_bool_t allow_dyn  = cais_sandbox_policy_get_allow_dynamic_code(policy);

        if (!allow_proc) {
            CAIS_WIN_CHILD_PROCESS_POLICY cp;
            memset(&cp, 0, sizeof(cp));
            cp.Flags = 1; /* NoChildProcessCreation = 1 */
            if (!s_pfn_SetProcessMitigationPolicy(CAIS_WIN_PROCESS_CHILD_PROCESS_POLICY, &cp, sizeof(cp))) {
                cais_set_err_fmt(err_buf, err_cap, "failed to apply child process restriction (code %lu)", (unsigned long)GetLastError());
                return CAIS_SANDBOX_ERROR;
            }
        }

        if (!allow_dyn) {
            CAIS_WIN_DYNAMIC_CODE_POLICY dp;
            memset(&dp, 0, sizeof(dp));
            dp.Flags = 1; /* ProhibitDynamicCode = 1 */
            if (!s_pfn_SetProcessMitigationPolicy(CAIS_WIN_PROCESS_DYNAMIC_CODE_POLICY, &dp, sizeof(dp))) {
                cais_set_err_fmt(err_buf, err_cap, "failed to apply dynamic code restriction (code %lu)", (unsigned long)GetLastError());
                return CAIS_SANDBOX_ERROR;
            }
        }

        {
            CAIS_WIN_STRICT_HANDLE_CHECK_POLICY shp;
            memset(&shp, 0, sizeof(shp));
            shp.Flags = 1 | 2; /* RaiseExceptionOnInvalidHandleReference | HandleExceptionsPermanentlyEnabled */
            s_pfn_SetProcessMitigationPolicy(CAIS_WIN_PROCESS_STRICT_HANDLE_CHECK_POLICY, &shp, sizeof(shp));
        }
    } else {
        /* Total lockdown */
        CAIS_WIN_CHILD_PROCESS_POLICY cp;
        CAIS_WIN_DYNAMIC_CODE_POLICY dp;
        CAIS_WIN_STRICT_HANDLE_CHECK_POLICY shp;

        memset(&cp, 0, sizeof(cp));
        cp.Flags = 1;
        s_pfn_SetProcessMitigationPolicy(CAIS_WIN_PROCESS_CHILD_PROCESS_POLICY, &cp, sizeof(cp));

        memset(&dp, 0, sizeof(dp));
        dp.Flags = 1;
        s_pfn_SetProcessMitigationPolicy(CAIS_WIN_PROCESS_DYNAMIC_CODE_POLICY, &dp, sizeof(dp));

        memset(&shp, 0, sizeof(shp));
        shp.Flags = 1 | 2;
        s_pfn_SetProcessMitigationPolicy(CAIS_WIN_PROCESS_STRICT_HANDLE_CHECK_POLICY, &shp, sizeof(shp));
    }

    return CAIS_SANDBOX_OK;
}

#endif /* CAIS_IS_WINDOWS || defined(_WIN32) */
