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

#if !CAIS_IS_POSIX && !CAIS_IS_WINDOWS && !CAIS_IS_AMIGA && !CAIS_IS_DOS && !CAIS_IS_OS2 && !CAIS_IS_PLAN9 && !CAIS_IS_KOLIBRI && !CAIS_IS_ATARIST

#include <stdlib.h>
#include <string.h>

#include <cais/socket.h>
#include "socket_internal.h"

cais_socket_t *cais_socket_create(cais_socket_type_t type, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    (void)type;
    (void)mode;
    cais_set_err(err_buf, err_cap, "sockets are not supported on this platform");
    return NULL;
}

void cais_socket_backend_close(cais_socket_t *sock)
{
    (void)sock;
}

cais_socket_status_t cais_socket_bind_sock(cais_socket_t *sock, const char *host, int port, int backlog, char *err_buf, size_t err_cap)
{
    (void)sock;
    (void)host;
    (void)port;
    (void)backlog;
    cais_set_err(err_buf, err_cap, "sockets are not supported on this platform");
    return CAIS_SOCK_ERROR;
}

cais_socket_status_t cais_socket_connect_sock(cais_socket_t *sock, const char *host, int port, char *err_buf, size_t err_cap)
{
    (void)sock;
    (void)host;
    (void)port;
    cais_set_err(err_buf, err_cap, "sockets are not supported on this platform");
    return CAIS_SOCK_ERROR;
}

cais_socket_t *cais_socket_accept(cais_socket_t *server, char *client_ip, size_t ip_len,
                                  int *client_port, char *err_buf, size_t err_cap)
{
    (void)server;
    (void)client_ip;
    (void)ip_len;
    (void)client_port;
    cais_set_err(err_buf, err_cap, "sockets are not supported on this platform");
    return NULL;
}

cais_socket_status_t cais_socket_backend_send(cais_socket_t *sock, const char *buf, size_t len, size_t *sent)
{
    (void)sock;
    (void)buf;
    (void)len;
    if (sent != NULL) {
        *sent = 0;
    }
    return CAIS_SOCK_ERROR;
}

cais_socket_status_t cais_socket_recv(cais_socket_t *sock, char *buf, size_t max_len, size_t *received)
{
    (void)sock;
    (void)buf;
    (void)max_len;
    if (received != NULL) {
        *received = 0;
    }
    return CAIS_SOCK_ERROR;
}

cais_socket_status_t cais_socket_sendto(cais_socket_t *sock, const char *buf, size_t len,
                                        const char *host, int port, size_t *sent)
{
    (void)sock;
    (void)buf;
    (void)len;
    (void)host;
    (void)port;
    if (sent != NULL) {
        *sent = 0;
    }
    return CAIS_SOCK_ERROR;
}

cais_socket_status_t cais_socket_recvfrom(cais_socket_t *sock, char *buf, size_t max_len,
                                          char *src_ip, size_t ip_len, int *src_port,
                                          size_t *received)
{
    (void)sock;
    (void)buf;
    (void)max_len;
    (void)src_ip;
    (void)ip_len;
    (void)src_port;
    if (received != NULL) {
        *received = 0;
    }
    return CAIS_SOCK_ERROR;
}

cais_socket_status_t cais_socket_backend_set_mode(cais_socket_t *sock, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    (void)sock;
    (void)mode;
    (void)err_buf;
    (void)err_cap;
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_backend_set_timeout(cais_socket_t *sock, int timeout_ms, char *err_buf, size_t err_cap)
{
    (void)sock;
    (void)timeout_ms;
    (void)err_buf;
    (void)err_cap;
    return CAIS_SOCK_OK;
}

int cais_socket_backend_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms)
{
    (void)fds;
    (void)nfds;
    (void)timeout_ms;
    return 0;
}

#else
/* ISO C forbids an empty translation unit */
typedef int make_iso_compilers_happy_socket_stub;
#endif /* stub */
