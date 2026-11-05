/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_SOCKET_INTERNAL_H
#define CAIS_SOCKET_INTERNAL_H

#include <cais/platform.h>
#include <cais/socket.h>

#include <stddef.h>

#if CAIS_IS_WINDOWS
#include <winsock2.h>
#include <windows.h>
#define CAIS_SOCK_HANDLE_TYPE SOCKET
#define CAIS_INVALID_SOCK_HANDLE INVALID_SOCKET
#else
#define CAIS_SOCK_HANDLE_TYPE int
#define CAIS_INVALID_SOCK_HANDLE (-1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Internal structure definition for cais_socket.
 * Shared across core engine and all OS platform drivers.
 */
struct cais_socket {
#if CAIS_IS_PLAN9
    int data_fd;      /* /net/<proto>/<conv_id>/data */
    int ctl_fd;       /* /net/<proto>/<conv_id>/ctl */
    char conv_id[16]; /* Conversation line number (e.g. "42") */
    int is_server;    /* 1 if server socket, 0 if client */
#else
    CAIS_SOCK_HANDLE_TYPE fd;
#endif
    cais_socket_type_t type;
    cais_socket_mode_t mode;
    int timeout_ms;
};

#if CAIS_IS_PLAN9
#define CAIS_SOCK_IS_VALID(s) ((s) != NULL && (s)->data_fd >= 0)
#define CAIS_SOCK_FD(s)       ((s) ? (s)->data_fd : -1)
#elif CAIS_IS_WINDOWS
#define CAIS_SOCK_IS_VALID(s) ((s) != NULL && (s)->fd != INVALID_SOCKET)
#define CAIS_SOCK_FD(s)       ((s) ? (int)(s)->fd : -1)
#else
#define CAIS_SOCK_IS_VALID(s) ((s) != NULL && (s)->fd >= 0)
#define CAIS_SOCK_FD(s)       ((s) ? (s)->fd : -1)
#endif

/* =============================================================================
 * Shared Address Engine Helpers
 * ============================================================================= */
const char *cais_socket_normalize_bind_host(const char *host);
const char *cais_socket_normalize_connect_host(const char *host);
int cais_socket_parse_ipv4(const char *host, unsigned long *out_ip_be);
int cais_socket_format_ipv4(unsigned long ip_be, char *buf, size_t len);
int cais_socket_format_port(int port, char *buf, size_t len);

/* =============================================================================
 * Shared Timeout Engine Helpers
 * ============================================================================= */
int cais_socket_normalize_timeout(int timeout_ms);
void cais_socket_ms_to_timeval(int timeout_ms, long *sec, long *usec);

/* =============================================================================
 * Shared Error Reporting Helpers (Safe C89 Caller Buffers)
 * ============================================================================= */
#define cais_socket_set_err     cais_set_err
#define cais_socket_set_err_fmt cais_set_err_fmt

/* =============================================================================
 * Driver SPI (Service Provider Interface) for OS Backends
 * ============================================================================= */
void cais_socket_backend_close(cais_socket_t *sock);
cais_socket_status_t cais_socket_backend_send(cais_socket_t *sock, const char *buf, size_t len, size_t *sent);
cais_socket_status_t cais_socket_backend_set_mode(cais_socket_t *sock, cais_socket_mode_t mode, char *err_buf, size_t err_cap);
cais_socket_status_t cais_socket_backend_set_timeout(cais_socket_t *sock, int timeout_ms, char *err_buf, size_t err_cap);
int cais_socket_backend_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_SOCKET_INTERNAL_H */
