/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "tls_internal.h"
#include <string.h>

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

static void set_stub_error(char *err_buf, size_t err_cap)
{
    if (err_buf && err_cap > 0) {
        strncpy(err_buf, "TLS subsystem not supported in this build", err_cap - 1);
        err_buf[err_cap - 1] = '\0';
    }
}

static int stub_init(cais_tls_t *tls, const cais_tls_config_t *conf, char *err_buf, size_t err_cap)
{
    (void)tls;
    (void)conf;
    set_stub_error(err_buf, err_cap);
    return CAIS_TLS_ERROR;
}

static int stub_handshake(cais_tls_t *tls, char *err_buf, size_t err_cap)
{
    (void)tls;
    set_stub_error(err_buf, err_cap);
    return CAIS_TLS_ERROR;
}

static int stub_send(cais_tls_t *tls, const void *data, size_t len, size_t *written_bytes, char *err_buf, size_t err_cap)
{
    (void)tls;
    (void)data;
    (void)len;
    if (written_bytes) *written_bytes = 0;
    set_stub_error(err_buf, err_cap);
    return CAIS_TLS_ERROR;
}

static int stub_recv(cais_tls_t *tls, void *buffer, size_t len, size_t *read_bytes, char *err_buf, size_t err_cap)
{
    (void)tls;
    (void)buffer;
    (void)len;
    if (read_bytes) *read_bytes = 0;
    set_stub_error(err_buf, err_cap);
    return CAIS_TLS_ERROR;
}

static int stub_close(cais_tls_t *tls)
{
    (void)tls;
    return CAIS_TLS_OK;
}

static void stub_free(cais_tls_t *tls)
{
    (void)tls;
}

static const cais_tls_backend_ops_t s_stub_ops = {
    CAIS_TLS_BACKEND_NONE,
    "stub",
    stub_init,
    stub_handshake,
    stub_send,
    stub_recv,
    stub_close,
    stub_free
};

const cais_tls_backend_ops_t *cais_tls_get_backend_ops(void)
{
    return &s_stub_ops;
}
