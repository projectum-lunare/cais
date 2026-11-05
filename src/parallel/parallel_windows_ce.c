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

#if CAIS_IS_WINDOWS_CE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cais/parallel.h>

#include <windows.h>

struct parallel_port {
    HANDLE handle;
};

cais_parallel_t *cais_parallel_open(const char *path, char *err_buf, size_t err_cap)
{
    HANDLE h;
    cais_parallel_t *port;
    WCHAR target_path[64];
    char buf[64];
    size_t len;
    const char *p = (path && path[0] != '\0') ? path : "LPT1:";

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    len = strlen(p);
    if (len >= sizeof(buf) - 2)
        len = sizeof(buf) - 3;
    memcpy(buf, p, len);
    buf[len] = '\0';
    if (len > 0 && buf[len - 1] != ':') {
        buf[len] = ':';
        buf[len + 1] = '\0';
    }

    MultiByteToWideChar(CP_ACP, 0, buf, -1, target_path, sizeof(target_path) / sizeof(WCHAR));
    h = CreateFileW(target_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        cais_set_err_fmt(err_buf, err_cap, "CreateFileW(%s) failed (error %lu)",
                         path ? path : "LPT1:", (unsigned long)GetLastError());
        return NULL;
    }

    port = (cais_parallel_t *)malloc(sizeof(cais_parallel_t));
    if (port == NULL) {
        CloseHandle(h);
        cais_set_err(err_buf, err_cap, "Memory allocation failure");
        return NULL;
    }

    port->handle = h;
    return port;
}

cais_status_t cais_parallel_write(cais_parallel_t *port, const char *buf, size_t len, size_t *written_bytes)
{
    DWORD written = 0;
    size_t total = 0;

    if (written_bytes != NULL) {
        *written_bytes = 0;
    }

    if (port == NULL || port->handle == INVALID_HANDLE_VALUE || buf == NULL) {
        return CAIS_ERROR;
    }
    if (len == 0) {
        return CAIS_OK;
    }

    while (total < len) {
        DWORD chunk = (DWORD)(len - total);
        if (!WriteFile(port->handle, buf + total, chunk, &written, NULL)) {
            if (written_bytes != NULL) *written_bytes = total;
            return (total > 0) ? CAIS_OK : CAIS_ERROR;
        }
        if (written == 0) {
            break;
        }
        total += written;
    }

    if (written_bytes != NULL) {
        *written_bytes = total;
    }
    return CAIS_OK;
}

cais_status_t cais_parallel_read(cais_parallel_t *port, char *buf, size_t max_len, size_t *read_bytes)
{
    (void)port;
    (void)buf;
    (void)max_len;
    if (read_bytes != NULL) *read_bytes = 0;
    return CAIS_UNSUPPORTED;
}

cais_parallel_status_t cais_parallel_get_status(cais_parallel_t *port, int *pins_out)
{
    (void)port;
    if (pins_out != NULL) *pins_out = 0;
    return CAIS_PARALLEL_UNSUPPORTED;
}

cais_parallel_status_t cais_parallel_set_mode(cais_parallel_t *port, cais_parallel_mode_t mode)
{
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) return CAIS_PARALLEL_ERROR;
    if (mode == CAIS_PARALLEL_MODE_SPP) return CAIS_PARALLEL_OK;
    return CAIS_PARALLEL_UNSUPPORTED;
}

cais_parallel_status_t cais_parallel_drain(cais_parallel_t *port)
{
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) {
        return CAIS_PARALLEL_ERROR;
    }
    return FlushFileBuffers(port->handle) ? CAIS_PARALLEL_OK : CAIS_PARALLEL_ERROR;
}

void cais_parallel_close(cais_parallel_t *port)
{
    if (port != NULL) {
        if (port->handle != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(port->handle);
            CloseHandle(port->handle);
            port->handle = INVALID_HANDLE_VALUE;
        }
        free(port);
    }
}

#else
/* ISO C forbids an empty translation unit */
typedef int make_iso_compilers_happy_parallel_windows_ce;
#endif /* CAIS_IS_WINDOWS_CE */
