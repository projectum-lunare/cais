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

#if !defined(CAIS_OS_HAS_NATIVE_DRIVERS)

#include <stdlib.h>
#include <string.h>

#include <cais/parallel.h>

struct parallel_port {
    int dummy;
};

cais_parallel_t *cais_parallel_open(const char *path, char *err_buf, size_t err_cap)
{
    (void)path;
    cais_set_err(err_buf, err_cap, "Parallel ports are not supported on this platform");
    return NULL;
}

cais_status_t cais_parallel_write(cais_parallel_t *port, const char *buf, size_t len, size_t *written_bytes)
{
    (void)port;
    (void)buf;
    (void)len;
    if (written_bytes != NULL) *written_bytes = 0;
    return CAIS_UNSUPPORTED;
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
    (void)port;
    (void)mode;
    return CAIS_PARALLEL_UNSUPPORTED;
}

cais_parallel_status_t cais_parallel_drain(cais_parallel_t *port)
{
    (void)port;
    return CAIS_PARALLEL_UNSUPPORTED;
}

void cais_parallel_close(cais_parallel_t *port)
{
    (void)port;
}

#else
/* ISO C forbids an empty translation unit */
typedef int make_iso_compilers_happy_parallel_stub;
#endif /* !CAIS_OS_HAS_NATIVE_DRIVERS */
