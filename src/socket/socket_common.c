/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "socket_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* =============================================================================
 * Shared Address Engine
 * ============================================================================= */

const char *cais_socket_normalize_bind_host(const char *host)
{
    if (host == NULL || host[0] == '\0' ||
        strcmp(host, "0.0.0.0") == 0 || strcmp(host, "*") == 0) {
        return NULL;
    }
    return host;
}

const char *cais_socket_normalize_connect_host(const char *host)
{
    if (host == NULL || host[0] == '\0') {
        return "127.0.0.1";
    }
    return host;
}

int cais_socket_parse_ipv4(const char *host, unsigned long *out_ip_be)
{
    unsigned int a, b, c, d;
    char tail[2];

    if (host == NULL || host[0] == '\0') {
        return 0;
    }

    if (sscanf(host, "%u.%u.%u.%u%1s", &a, &b, &c, &d, tail) == 4) {
        if (a <= 255 && b <= 255 && c <= 255 && d <= 255) {
            if (out_ip_be != NULL) {
                *out_ip_be = (((unsigned long)a) << 24) |
                             (((unsigned long)b) << 16) |
                             (((unsigned long)c) << 8)  |
                             ((unsigned long)d);
            }
            return 1;
        }
    }
    return 0;
}

int cais_socket_format_ipv4(unsigned long ip_be, char *buf, size_t len)
{
    unsigned int a = (unsigned int)((ip_be >> 24) & 0xFF);
    unsigned int b = (unsigned int)((ip_be >> 16) & 0xFF);
    unsigned int c = (unsigned int)((ip_be >> 8) & 0xFF);
    unsigned int d = (unsigned int)(ip_be & 0xFF);
    int n;

    if (buf == NULL || len == 0) {
        return -1;
    }

    n = sprintf(buf, "%u.%u.%u.%u", a, b, c, d);
    return (n > 0 && (size_t)n < len) ? 0 : -1;
}

int cais_socket_format_port(int port, char *buf, size_t len)
{
    int n;
    if (buf == NULL || len == 0 || port < 0 || port > 65535) {
        return -1;
    }
    n = sprintf(buf, "%d", port);
    return (n > 0 && (size_t)n < len) ? 0 : -1;
}

/* =============================================================================
 * Shared Timeout Engine
 * ============================================================================= */

int cais_socket_normalize_timeout(int timeout_ms)
{
    return (timeout_ms < 0) ? 0 : timeout_ms;
}

void cais_socket_ms_to_timeval(int timeout_ms, long *sec, long *usec)
{
    if (timeout_ms < 0) {
        timeout_ms = 0;
    }
    if (sec != NULL) {
        *sec = timeout_ms / 1000;
    }
    if (usec != NULL) {
        *usec = (long)(timeout_ms % 1000) * 1000L;
    }
}

/* =============================================================================
 * Lifecycle & Creation Wrappers
 * ============================================================================= */

cais_socket_t *cais_socket_tcp(cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    return cais_socket_create(CAIS_SOCK_TCP, mode, err_buf, err_cap);
}

cais_socket_t *cais_socket_udp(cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    return cais_socket_create(CAIS_SOCK_UDP, mode, err_buf, err_cap);
}

void cais_socket_close(cais_socket_t *sock)
{
    if (sock != NULL) {
        cais_socket_backend_close(sock);
        free(sock);
    }
}

cais_socket_t *cais_socket_bind(const char *host, int port, int backlog, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    cais_socket_t *sock;
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }
    sock = cais_socket_tcp(mode, err_buf, err_cap);
    if (sock == NULL) {
        return NULL;
    }

    if (cais_socket_bind_sock(sock, host, port, backlog, err_buf, err_cap) != CAIS_SOCK_OK) {
        cais_socket_close(sock);
        return NULL;
    }

    return sock;
}

cais_socket_t *cais_socket_udp_bind(const char *host, int port, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    cais_socket_t *sock;
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }
    sock = cais_socket_udp(mode, err_buf, err_cap);
    if (sock == NULL) {
        return NULL;
    }

    if (cais_socket_bind_sock(sock, host, port, 0, err_buf, err_cap) != CAIS_SOCK_OK) {
        cais_socket_close(sock);
        return NULL;
    }

    return sock;
}

cais_socket_t *cais_socket_connect(const char *host, int port, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    cais_socket_status_t st;
    cais_socket_t *sock;
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }
    sock = cais_socket_tcp(mode, err_buf, err_cap);
    if (sock == NULL) {
        return NULL;
    }

    st = cais_socket_connect_sock(sock, host, port, err_buf, err_cap);
    if (st != CAIS_SOCK_OK && st != CAIS_SOCK_AGAIN) {
        cais_socket_close(sock);
        return NULL;
    }

    return sock;
}

/* =============================================================================
 * Stream & Datagram I/O
 * ============================================================================= */

cais_socket_status_t cais_socket_write(cais_socket_t *sock, const char *buf, size_t len, size_t *sent)
{
    return cais_socket_send(sock, buf, len, sent);
}

cais_socket_status_t cais_socket_read(cais_socket_t *sock, char *buf, size_t max_len, size_t *received)
{
    return cais_socket_recv(sock, buf, max_len, received);
}

cais_socket_status_t cais_socket_send(cais_socket_t *sock, const char *buf, size_t len, size_t *sent)
{
    size_t total = 0;
    if (sent != NULL) {
        *sent = 0;
    }
    if (!CAIS_SOCK_IS_VALID(sock) || buf == NULL) {
        return CAIS_SOCK_ERROR;
    }
    if (len == 0) {
        return CAIS_SOCK_OK;
    }

    /* Datagram (UDP) or non-blocking: execute single raw send attempt */
    if (sock->mode == CAIS_SOCK_NONBLOCKING || sock->type == CAIS_SOCK_UDP) {
        size_t n = 0;
        cais_socket_status_t st = cais_socket_backend_send(sock, buf, len, &n);
        if (st == CAIS_SOCK_OK && sent != NULL) {
            *sent = n;
        }
        return st;
    }

    /* Blocking TCP stream send: loop ensuring complete delivery of all bytes */
    while (total < len) {
        size_t chunk = 0;
        cais_socket_status_t st = cais_socket_backend_send(sock, buf + total, len - total, &chunk);
        if (st == CAIS_SOCK_OK) {
            if (chunk == 0) {
                break;
            }
            total += chunk;
        } else if (st == CAIS_SOCK_AGAIN) {
            if (sock->timeout_ms > 0) {
                int ev = cais_socket_wait(sock, CAIS_SOCK_POLL_WRITE, sock->timeout_ms);
                if (ev <= 0) {
                    if (sent != NULL) {
                        *sent = total;
                    }
                    return (ev == 0) ? CAIS_SOCK_AGAIN : CAIS_SOCK_ERROR;
                }
            }
            continue;
        } else {
            if (sent != NULL) {
                *sent = total;
            }
            return CAIS_SOCK_ERROR;
        }
    }

    if (sent != NULL) {
        *sent = total;
    }
    return CAIS_SOCK_OK;
}

/* =============================================================================
 * Mode, Timeout, Polling & Properties
 * ============================================================================= */

cais_socket_mode_t cais_socket_get_mode(const cais_socket_t *sock)
{
    return sock ? sock->mode : CAIS_SOCK_BLOCKING;
}

cais_bool_t cais_socket_is_blocking(const cais_socket_t *sock)
{
    if (sock == NULL) return CAIS_FALSE;
    return (sock->mode == CAIS_SOCK_BLOCKING) ? CAIS_TRUE : CAIS_FALSE;
}

cais_bool_t cais_socket_is_nonblocking(const cais_socket_t *sock)
{
    if (sock == NULL) return CAIS_FALSE;
    return (sock->mode == CAIS_SOCK_NONBLOCKING) ? CAIS_TRUE : CAIS_FALSE;
}

cais_socket_type_t cais_socket_get_type(const cais_socket_t *sock)
{
    return sock ? sock->type : CAIS_SOCK_TCP;
}

int cais_socket_get_fd(const cais_socket_t *sock)
{
    return CAIS_SOCK_FD(sock);
}

cais_socket_status_t cais_socket_set_mode(cais_socket_t *sock, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }
    if (!CAIS_SOCK_IS_VALID(sock)) {
        cais_socket_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }
    if (cais_socket_backend_set_mode(sock, mode, err_buf, err_cap) != CAIS_SOCK_OK) {
        return CAIS_SOCK_ERROR;
    }
    sock->mode = mode;
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_set_timeout(cais_socket_t *sock, int timeout_ms, char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }
    if (!CAIS_SOCK_IS_VALID(sock)) {
        cais_socket_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }
    timeout_ms = cais_socket_normalize_timeout(timeout_ms);
    if (cais_socket_backend_set_timeout(sock, timeout_ms, err_buf, err_cap) != CAIS_SOCK_OK) {
        return CAIS_SOCK_ERROR;
    }
    sock->timeout_ms = timeout_ms;
    return CAIS_SOCK_OK;
}

int cais_socket_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms)
{
    if (fds == NULL && nfds > 0) {
        return CAIS_SOCK_ERROR;
    }
    return cais_socket_backend_poll(fds, nfds, timeout_ms);
}

int cais_socket_wait(cais_socket_t *sock, int events, int timeout_ms)
{
    cais_pollfd_t pfd;
    int ret;

    if (!CAIS_SOCK_IS_VALID(sock)) {
        return CAIS_SOCK_ERROR;
    }

    pfd.sock = sock;
    pfd.events = events;
    pfd.revents = 0;

    ret = cais_socket_poll(&pfd, 1, timeout_ms);
    if (ret < 0) {
        return CAIS_SOCK_ERROR;
    }
    if (ret == 0) {
        return 0; /* Timeout */
    }
    return pfd.revents;
}
