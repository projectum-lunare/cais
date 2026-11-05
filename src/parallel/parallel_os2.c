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

#if CAIS_IS_OS2

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <cais/parallel.h>

struct parallel_port {
    HFILE handle;
    char name[16];
};

cais_parallel_t *cais_parallel_open(const char *path, char *err_buf, size_t err_cap)
{
    const char *dev_name = (path && path[0] != '\0') ? path : "LPT1";
    HFILE hfile = NULLHANDLE;
    ULONG action = 0;
    APIRET rc;
    cais_parallel_t *port;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    /*
     * DosOpen with:
     * - OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW: open device if it exists
     * - OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_WRITEONLY: exclusive write access
     */
    rc = DosOpen((PCSZ)dev_name, &hfile, &action, 0, FILE_NORMAL,
                 OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                 OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_WRITEONLY, NULL);

    if (rc != 0) {
        cais_set_err_fmt(err_buf, err_cap, "DosOpen(%s) failed (SYS%04lu)", dev_name,
                         (unsigned long)rc);
        return NULL;
    }

    port = (cais_parallel_t *)malloc(sizeof(cais_parallel_t));
    if (port == NULL) {
        DosClose(hfile);
        cais_set_err(err_buf, err_cap, "Out of memory allocating parallel port structure");
        return NULL;
    }

    port->handle = hfile;
    strncpy(port->name, dev_name, sizeof(port->name) - 1);
    port->name[sizeof(port->name) - 1] = '\0';

    return port;
}

cais_status_t cais_parallel_write(cais_parallel_t *port, const char *buf, size_t len, size_t *written_bytes)
{
    ULONG written = 0;
    APIRET rc;

    if (written_bytes != NULL) {
        *written_bytes = 0;
    }

    if (port == NULL || port->handle == NULLHANDLE || buf == NULL) {
        return CAIS_ERROR;
    }
    if (len == 0) {
        return CAIS_OK;
    }

    rc = DosWrite(port->handle, (PVOID)buf, (ULONG)len, &written);
    if (rc != 0) {
        return CAIS_ERROR;
    }

    if (written_bytes != NULL) {
        *written_bytes = (size_t)written;
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
    if (port == NULL || port->handle == NULLHANDLE) return CAIS_PARALLEL_ERROR;
    if (mode == CAIS_PARALLEL_MODE_SPP) return CAIS_PARALLEL_OK;
    return CAIS_PARALLEL_UNSUPPORTED;
}

cais_parallel_status_t cais_parallel_drain(cais_parallel_t *port)
{
    if (port == NULL || port->handle == NULLHANDLE) {
        return CAIS_PARALLEL_ERROR;
    }

    /*
     * DosResetBuffer flushes all internal file/device buffers to the
     * attached parallel printer device.
     */
    DosResetBuffer(port->handle);
    return CAIS_PARALLEL_OK;
}

void cais_parallel_close(cais_parallel_t *port)
{
    if (port != NULL) {
        if (port->handle != NULLHANDLE) {
            DosClose(port->handle);
            port->handle = NULLHANDLE;
        }
        free(port);
    }
}

#else
/* ISO C forbids an empty translation unit */
typedef int make_iso_compilers_happy_parallel_os2;
#endif /* CAIS_IS_OS2 */
