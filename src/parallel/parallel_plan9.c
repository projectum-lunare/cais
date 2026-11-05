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

#if CAIS_IS_PLAN9

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cais/parallel.h>

#include <fcntl.h>
#include <unistd.h>

struct parallel_port {
    int fd;
};

cais_parallel_t *cais_parallel_open(const char *path, char *err_buf, size_t err_cap)
{
    const char *target_path = (path && path[0] != '\0') ? path : "/dev/lpt";
    int fd;
    cais_parallel_t *port;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    fd = open(target_path, O_WRONLY);
    if (fd < 0 && strncmp(target_path, "/dev/lpt", 8) == 0) {
        /* Fallback to device space #p/lpt if #p is not bound to /dev */
        char alt_path[128];
        snprintf(alt_path, sizeof(alt_path), "#p/%s", target_path + 5);
        fd = open(alt_path, O_WRONLY);
        if (fd >= 0) {
            target_path = alt_path;
        }
    }

    if (fd < 0) {
        cais_set_err_fmt(err_buf, err_cap, "Cannot open Plan 9 parallel device '%s' (%s)", target_path,
                         strerror(errno));
        return NULL;
    }

    port = (cais_parallel_t *)malloc(sizeof(cais_parallel_t));
    if (port == NULL) {
        close(fd);
        cais_set_err(err_buf, err_cap, "Memory allocation failure");
        return NULL;
    }

    port->fd = fd;
    return port;
}

cais_status_t cais_parallel_write(cais_parallel_t *port, const char *buf, size_t len, size_t *written_bytes)
{
    ssize_t written;
    size_t total = 0;

    if (written_bytes != NULL) {
        *written_bytes = 0;
    }

    if (port == NULL || port->fd < 0 || buf == NULL) {
        return CAIS_ERROR;
    }
    if (len == 0) {
        return CAIS_OK;
    }

    while (total < len) {
        written = write(port->fd, buf + total, len - total);
        if (written < 0) {
            if (written_bytes != NULL) *written_bytes = total;
            return (total > 0) ? CAIS_OK : CAIS_ERROR;
        }
        if (written == 0) {
            break;
        }
        total += (size_t)written;
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
    if (port == NULL || port->fd < 0) return CAIS_PARALLEL_ERROR;
    if (mode == CAIS_PARALLEL_MODE_SPP) return CAIS_PARALLEL_OK;
    return CAIS_PARALLEL_UNSUPPORTED;
}

cais_parallel_status_t cais_parallel_drain(cais_parallel_t *port)
{
    if (port == NULL || port->fd < 0)
        return CAIS_PARALLEL_ERROR;
    fsync(port->fd);
    return CAIS_PARALLEL_OK;
}

void cais_parallel_close(cais_parallel_t *port)
{
    if (port != NULL) {
        if (port->fd >= 0) {
            fsync(port->fd);
            close(port->fd);
            port->fd = -1;
        }
        free(port);
    }
}

#else
/* ISO C forbids an empty translation unit */
typedef int make_iso_compilers_happy_parallel_plan9;
#endif /* CAIS_IS_PLAN9 */
