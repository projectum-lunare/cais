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

#if CAIS_IS_ATARIST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osbind.h>

#include <cais/parallel.h>

struct parallel_port {
    short dev_id; /* BIOS device 0 = PRT: */
    char name[16];
};

cais_parallel_t *cais_parallel_open(const char *path, char *err_buf, size_t err_cap)
{
    cais_parallel_t *port;
    (void)path;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    port = (cais_parallel_t *)malloc(sizeof(cais_parallel_t));
    if (port == NULL) {
        cais_set_err(err_buf, err_cap, "Out of memory allocating parallel port handle");
        return NULL;
    }

    port->dev_id = 0; /* BIOS Device 0: Printer (PRT:) */
    strncpy(port->name, (path && path[0] != '\0') ? path : "PRT:", sizeof(port->name) - 1);
    port->name[sizeof(port->name) - 1] = '\0';

    return port;
}

cais_status_t cais_parallel_write(cais_parallel_t *port, const char *buf, size_t len, size_t *written_bytes)
{
    size_t i;

    if (written_bytes != NULL) {
        *written_bytes = 0;
    }

    if (port == NULL || buf == NULL) {
        return CAIS_ERROR;
    }
    if (len == 0) {
        return CAIS_OK;
    }

    for (i = 0; i < len; i++) {
        /*
         * Poll Bcostat(0) until printer is ready to accept a byte.
         * The Atari ST sends parallel data via the Yamaha YM2149 sound chip
         * and monitors BUSY via the MFP 68901.
         * If the printer is powered off or offline, timeout after a delay.
         */
        long timeout = 2000000L;
        while (Bcostat(port->dev_id) == 0) {
            if (--timeout <= 0) {
                if (written_bytes != NULL) *written_bytes = i;
                return (i > 0) ? CAIS_OK : CAIS_ERROR;
            }
        }

        Bconout(port->dev_id, (unsigned char)buf[i]);
    }

    if (written_bytes != NULL) {
        *written_bytes = len;
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
    if (port == NULL) return CAIS_PARALLEL_ERROR;
    if (mode == CAIS_PARALLEL_MODE_SPP) return CAIS_PARALLEL_OK;
    return CAIS_PARALLEL_UNSUPPORTED;
}

cais_parallel_status_t cais_parallel_drain(cais_parallel_t *port)
{
    long timeout = 2000000L;

    if (port == NULL) {
        return CAIS_PARALLEL_ERROR;
    }

    /* Wait for hardware BUSY line to clear */
    while (Bcostat(port->dev_id) == 0) {
        if (--timeout <= 0) {
            return CAIS_PARALLEL_ERROR;
        }
    }

    return CAIS_PARALLEL_OK;
}

void cais_parallel_close(cais_parallel_t *port)
{
    if (port != NULL) {
        free(port);
    }
}

#else
/* ISO C forbids an empty translation unit */
typedef int make_iso_compilers_happy_parallel_atarist;
#endif /* CAIS_IS_ATARIST */
