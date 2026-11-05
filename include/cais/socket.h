/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_SOCKET_H
#define CAIS_SOCKET_H

#include <stddef.h>
#include <cais/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cais_socket cais_socket_t;

/*
 * Socket transport / protocol type
 */
typedef enum cais_socket_type {
    CAIS_SOCK_TCP = 1,
    CAIS_SOCK_UDP = 2
} cais_socket_type_t;

#define CAIS_SOCK_STREAM CAIS_SOCK_TCP
#define CAIS_SOCK_DGRAM  CAIS_SOCK_UDP

/*
 * Socket blocking mode (Synchronous vs Asynchronous)
 */
typedef enum cais_socket_mode {
    CAIS_SOCK_BLOCKING    = 0,
    CAIS_SOCK_NONBLOCKING = 1
} cais_socket_mode_t;

/*
 * Readiness polling events
 */
typedef enum cais_socket_event {
    CAIS_SOCK_POLL_READ  = 1 << 0,
    CAIS_SOCK_POLL_WRITE = 1 << 1,
    CAIS_SOCK_POLL_ERR   = 1 << 2
} cais_socket_event_t;

/*
 * Scope & Protocol Foundation:
 * The cais.socket subsystem is explicitly designed as a portable IPv4 (32-bit)
 * networking engine, ensuring 100% API and semantic parity between modern
 * operating systems and vintage platforms (DOS WATT-32, OS/2, Amiga AmiTCP).
 *
 * Error Reporting & Zero Heap Allocation:
 * Functions accepting `char *err_buf, size_t err_cap` will write descriptive error
 * text into err_buf (if err_buf != NULL and err_cap > 0).
 * No heap memory is allocated by the library for error messages.
 * If err_buf is NULL or err_cap is 0, error text is omitted and functions report
 * failures solely through status codes or NULL pointers.
 */

/*
 * Lifecycle & Creation
 * On error, returns NULL and populates err_buf if provided.
 */
cais_socket_t *cais_socket_create(cais_socket_type_t type, cais_socket_mode_t mode, char *err_buf, size_t err_cap);
cais_socket_t *cais_socket_tcp(cais_socket_mode_t mode, char *err_buf, size_t err_cap);
cais_socket_t *cais_socket_udp(cais_socket_mode_t mode, char *err_buf, size_t err_cap);
void cais_socket_close(cais_socket_t *sock);

/*
 * Quick Setup (Connect & Bind)
 * On error, returns NULL and populates err_buf if provided.
 */
cais_socket_t *cais_socket_connect(const char *host, int port, cais_socket_mode_t mode, char *err_buf, size_t err_cap);
cais_socket_t *cais_socket_bind(const char *host, int port, int backlog, cais_socket_mode_t mode, char *err_buf, size_t err_cap);
cais_socket_t *cais_socket_udp_bind(const char *host, int port, cais_socket_mode_t mode, char *err_buf, size_t err_cap);

/*
 * Operations on existing socket
 * On error, returns CAIS_SOCK_ERROR (or NULL for accept) and populates err_buf if provided.
 */
cais_socket_status_t cais_socket_bind_sock(cais_socket_t *sock, const char *host, int port, int backlog, char *err_buf, size_t err_cap);
cais_socket_status_t cais_socket_connect_sock(cais_socket_t *sock, const char *host, int port, char *err_buf, size_t err_cap);
cais_socket_t *cais_socket_accept(cais_socket_t *server, char *client_ip, size_t ip_len, int *client_port, char *err_buf, size_t err_cap);

/*
 * Stream & Connected Datagram I/O
 * Returns CAIS_SOCK_OK with *sent and *received populated (if non-NULL),
 * CAIS_SOCK_AGAIN if non-blocking operation would block,
 * CAIS_SOCK_CLOSED on orderly peer disconnection (EOF),
 * or CAIS_SOCK_ERROR on failure.
 */
cais_socket_status_t cais_socket_send(cais_socket_t *sock, const char *buf, size_t len, size_t *sent);
cais_socket_status_t cais_socket_recv(cais_socket_t *sock, char *buf, size_t max_len, size_t *received);
cais_socket_status_t cais_socket_write(cais_socket_t *sock, const char *buf, size_t len, size_t *sent);
cais_socket_status_t cais_socket_read(cais_socket_t *sock, char *buf, size_t max_len, size_t *received);

/*
 * Datagram (UDP) I/O
 */
cais_socket_status_t cais_socket_sendto(cais_socket_t *sock, const char *buf, size_t len,
                                        const char *host, int port, size_t *sent);
cais_socket_status_t cais_socket_recvfrom(cais_socket_t *sock, char *buf, size_t max_len,
                                          char *src_ip, size_t ip_len, int *src_port,
                                          size_t *received);

/*
 * Mode, Timeout & Polling
 * On error, functions accepting err_buf return CAIS_SOCK_ERROR and populate err_buf if provided.
 */
/**
 * @brief Polling descriptor structure for multi-socket event multiplexing.
 */
typedef struct cais_pollfd {
    cais_socket_t *sock;     /**< Pointer to active socket to poll */
    int            events;   /**< Bitmask of requested input events (CAIS_SOCK_POLL_READ, CAIS_SOCK_POLL_WRITE) */
    int            revents;  /**< Bitmask of returned output events detected by cais_socket_poll() */
} cais_pollfd_t;

cais_socket_status_t cais_socket_set_mode(cais_socket_t *sock, cais_socket_mode_t mode, char *err_buf, size_t err_cap);
cais_socket_mode_t cais_socket_get_mode(const cais_socket_t *sock);
cais_bool_t cais_socket_is_blocking(const cais_socket_t *sock);
cais_bool_t cais_socket_is_nonblocking(const cais_socket_t *sock);
cais_socket_type_t cais_socket_get_type(const cais_socket_t *sock);
cais_socket_status_t cais_socket_set_timeout(cais_socket_t *sock, int timeout_ms, char *err_buf, size_t err_cap);
int cais_socket_wait(cais_socket_t *sock, int events, int timeout_ms);
int cais_socket_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms);
int cais_socket_get_fd(const cais_socket_t *sock);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_SOCKET_H */
