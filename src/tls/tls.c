/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cais/tls.h>
#include "tls_internal.h"
#include <cais/socket.h>
#include <string.h>

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

/*
 * Default Socket I/O Adapters
 */
#ifndef CAIS_NO_SOCKET
static int tls_socket_read(void *user_data, void *buf, size_t len, size_t *read_bytes)
{
    cais_socket_t *sock = (cais_socket_t *)user_data;
    size_t received = 0;
    cais_socket_status_t status;

    if (!sock) return CAIS_TLS_ERROR;

    status = cais_socket_recv(sock, (char *)buf, len, &received);
    if (read_bytes) *read_bytes = received;

    if (status == CAIS_SOCK_OK) return CAIS_TLS_OK;
    if (status == CAIS_SOCK_AGAIN) return CAIS_TLS_WANT_READ;
    if (status == CAIS_SOCK_CLOSED) return CAIS_TLS_CLOSED;
    return CAIS_TLS_ERROR;
}

static int tls_socket_write(void *user_data, const void *buf, size_t len, size_t *written_bytes)
{
    cais_socket_t *sock = (cais_socket_t *)user_data;
    size_t sent = 0;
    cais_socket_status_t status;

    if (!sock) return CAIS_TLS_ERROR;

    status = cais_socket_send(sock, (const char *)buf, len, &sent);
    if (written_bytes) *written_bytes = sent;

    if (status == CAIS_SOCK_OK) return CAIS_TLS_OK;
    if (status == CAIS_SOCK_AGAIN) return CAIS_TLS_WANT_WRITE;
    if (status == CAIS_SOCK_CLOSED) return CAIS_TLS_CLOSED;
    return CAIS_TLS_ERROR;
}
#endif

/*
 * Introspection & Metadata API
 */
cais_tls_backend_type_t cais_tls_backend(void)
{
    const cais_tls_backend_ops_t *ops = cais_tls_get_backend_ops();
    return ops ? ops->type : CAIS_TLS_BACKEND_NONE;
}

const char *cais_tls_backend_name(void)
{
    const cais_tls_backend_ops_t *ops = cais_tls_get_backend_ops();
    return ops ? ops->name : "none";
}

int cais_tls_is_supported(void)
{
    const cais_tls_backend_ops_t *ops = cais_tls_get_backend_ops();
    return (ops && ops->type != CAIS_TLS_BACKEND_NONE) ? 1 : 0;
}

/*
 * Configuration Management
 */
void cais_tls_config_init(cais_tls_config_t *conf, cais_tls_role_t role)
{
    if (!conf) return;
    memset(conf, 0, sizeof(*conf));
    conf->role = role;
    conf->verify = (role == CAIS_TLS_CLIENT) ? CAIS_TLS_VERIFY_REQUIRED : CAIS_TLS_VERIFY_NONE;
}

/*
 * Lifecycle
 */
int cais_tls_init(cais_tls_t *tls, const cais_tls_config_t *conf, char *err_buf, size_t err_cap)
{
    const cais_tls_backend_ops_t *ops;

    if (!tls) {
        if (err_buf && err_cap > 0) {
            strncpy(err_buf, "TLS context pointer is NULL", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
        return CAIS_TLS_ERROR;
    }

    memset(tls, 0, sizeof(*tls));
    if (conf) {
        tls->config = *conf;
    } else {
        cais_tls_config_init(&tls->config, CAIS_TLS_CLIENT);
    }

    ops = cais_tls_get_backend_ops();
    if (!ops || !ops->init) {
        if (err_buf && err_cap > 0) {
            strncpy(err_buf, "No active TLS backend available", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
        return CAIS_TLS_ERROR;
    }

    return ops->init(tls, &tls->config, err_buf, err_cap);
}

int cais_tls_set_socket(cais_tls_t *tls, struct cais_socket *sock)
{
    if (!tls || !sock) return CAIS_TLS_ERROR;
#ifndef CAIS_NO_SOCKET
    tls->sock = sock;
    tls->read_cb = tls_socket_read;
    tls->write_cb = tls_socket_write;
    tls->io_user_data = sock;
    return CAIS_TLS_OK;
#else
    return CAIS_TLS_ERROR;
#endif
}

int cais_tls_set_io(cais_tls_t *tls, cais_tls_read_cb read_cb, cais_tls_write_cb write_cb, void *user_data)
{
    if (!tls || !read_cb || !write_cb) return CAIS_TLS_ERROR;
    tls->sock = NULL;
    tls->read_cb = read_cb;
    tls->write_cb = write_cb;
    tls->io_user_data = user_data;
    return CAIS_TLS_OK;
}

int cais_tls_handshake(cais_tls_t *tls, char *err_buf, size_t err_cap)
{
    const cais_tls_backend_ops_t *ops;

    if (!tls || (!tls->sock && !tls->read_cb)) {
        if (err_buf && err_cap > 0) {
            strncpy(err_buf, "TLS context not bound to transport I/O", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
        return CAIS_TLS_ERROR;
    }

    ops = cais_tls_get_backend_ops();
    if (!ops || !ops->handshake) return CAIS_TLS_ERROR;

    return ops->handshake(tls, err_buf, err_cap);
}

int cais_tls_send(cais_tls_t *tls, const void *data, size_t len, size_t *written_bytes, char *err_buf, size_t err_cap)
{
    const cais_tls_backend_ops_t *ops;

    if (!tls || !tls->handshake_done) {
        if (err_buf && err_cap > 0) {
            strncpy(err_buf, "TLS handshake not completed", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
        return CAIS_TLS_ERROR;
    }

    ops = cais_tls_get_backend_ops();
    if (!ops || !ops->send) return CAIS_TLS_ERROR;

    return ops->send(tls, data, len, written_bytes, err_buf, err_cap);
}

int cais_tls_recv(cais_tls_t *tls, void *buffer, size_t len, size_t *read_bytes, char *err_buf, size_t err_cap)
{
    const cais_tls_backend_ops_t *ops;

    if (!tls || !tls->handshake_done) {
        if (err_buf && err_cap > 0) {
            strncpy(err_buf, "TLS handshake not completed", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
        return CAIS_TLS_ERROR;
    }

    ops = cais_tls_get_backend_ops();
    if (!ops || !ops->recv) return CAIS_TLS_ERROR;

    return ops->recv(tls, buffer, len, read_bytes, err_buf, err_cap);
}

int cais_tls_close(cais_tls_t *tls)
{
    const cais_tls_backend_ops_t *ops;

    if (!tls || tls->closed) return CAIS_TLS_OK;

    ops = cais_tls_get_backend_ops();
    if (ops && ops->close) {
        ops->close(tls);
    }
    tls->closed = 1;
    return CAIS_TLS_OK;
}

void cais_tls_free(cais_tls_t *tls)
{
    const cais_tls_backend_ops_t *ops;

    if (!tls) return;

    ops = cais_tls_get_backend_ops();
    if (ops && ops->free) {
        ops->free(tls);
    }
    memset(tls, 0, sizeof(*tls));
}
