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

#include <cais/serial.h>

struct serial_port {
    int dummy;
};

cais_serial_t *cais_serial_open(const char *path, const cais_serial_opts_t *opts, char *err_buf, size_t err_cap)
{
    (void)path;
    (void)opts;
    cais_set_err(err_buf, err_cap, "Serial ports are not supported on this platform");
    return NULL;
}

cais_status_t cais_serial_read(cais_serial_t *port, char *buf, size_t max_len, size_t *read_bytes, int timeout_ms)
{
    (void)port;
    (void)buf;
    (void)max_len;
    (void)timeout_ms;
    if (read_bytes != NULL) {
        *read_bytes = 0;
    }
    return CAIS_UNSUPPORTED;
}

cais_serial_status_t cais_serial_set_timeout(cais_serial_t *port, int timeout_ms)
{
    (void)port;
    (void)timeout_ms;
    return CAIS_SERIAL_UNSUPPORTED;
}

cais_line_state_t cais_serial_get_cts(cais_serial_t *port)
{
    (void)port;
    return CAIS_LINE_ERROR;
}

cais_line_state_t cais_serial_get_dsr(cais_serial_t *port)
{
    (void)port;
    return CAIS_LINE_ERROR;
}

cais_line_state_t cais_serial_get_cd(cais_serial_t *port)
{
    (void)port;
    return CAIS_LINE_ERROR;
}

cais_line_state_t cais_serial_get_ri(cais_serial_t *port)
{
    (void)port;
    return CAIS_LINE_ERROR;
}

cais_serial_status_t cais_serial_set_rts(cais_serial_t *port, cais_serial_line_t state)
{
    (void)port;
    (void)state;
    return CAIS_SERIAL_UNSUPPORTED;
}

cais_serial_status_t cais_serial_set_dtr(cais_serial_t *port, cais_serial_line_t state)
{
    (void)port;
    (void)state;
    return CAIS_SERIAL_UNSUPPORTED;
}

cais_status_t cais_serial_write(cais_serial_t *port, const char *buf, size_t len, size_t *written_bytes)
{
    (void)port;
    (void)buf;
    (void)len;
    if (written_bytes != NULL) {
        *written_bytes = 0;
    }
    return CAIS_UNSUPPORTED;
}

cais_serial_status_t cais_serial_drain(cais_serial_t *port)
{
    (void)port;
    return CAIS_SERIAL_UNSUPPORTED;
}

void cais_serial_close(cais_serial_t *port)
{
    (void)port;
}

#else
/* ISO C forbids an empty translation unit */
typedef int make_iso_compilers_happy_serial_stub;
#endif /* !CAIS_OS_HAS_NATIVE_DRIVERS */
