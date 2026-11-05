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

#if CAIS_IS_KOLIBRI

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cais/socket.h>
#include "socket_internal.h"

/*
 * KolibriOS System Function 75: Network & Sockets API
 * Invoked via software interrupt: int $0x40 with EAX = 75
 */
#define KOLIBRI_SYSFN_SOCKET 75

#define KOLIBRI_FN_SOCKET_OPEN    0
#define KOLIBRI_FN_SOCKET_CLOSE   1
#define KOLIBRI_FN_SOCKET_BIND    2
#define KOLIBRI_FN_SOCKET_LISTEN  3
#define KOLIBRI_FN_SOCKET_CONNECT 4
#define KOLIBRI_FN_SOCKET_ACCEPT  5
#define KOLIBRI_FN_SOCKET_SEND    6
#define KOLIBRI_FN_SOCKET_RECV    7

#define KOLIBRI_AF_INET4    2
#define KOLIBRI_SOCK_STREAM 1
#define KOLIBRI_SOCK_DGRAM  2
#define KOLIBRI_IPPROTO_TCP 6
#define KOLIBRI_IPPROTO_UDP 17

#define KOLIBRI_MSG_DONTWAIT 0x40

#pragma pack(push, 1)
struct kolibri_sockaddr_in {
    uint16_t sin_family; /* KOLIBRI_AF_INET4 = 2 */
    uint16_t sin_port;   /* Port in network byte order */
    uint32_t sin_addr;   /* IPv4 in network byte order */
    uint8_t sin_zero[8];
};
#pragma pack(pop)

static uint16_t kolibri_htons(uint16_t val)
{
    return (uint16_t)(((val >> 8) & 0x00FF) | ((val << 8) & 0xFF00));
}

static uint16_t kolibri_ntohs(uint16_t val)
{
    return kolibri_htons(val);
}

static uint32_t kolibri_ntohl(uint32_t val)
{
    return ((val >> 24) & 0x000000FF) | ((val >> 8) & 0x0000FF00) | ((val << 8) & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}

/*
 * KolibriOS int 0x40 syscall wrappers for sysfn 75
 */
static int sys_socket_open(int domain, int type, int proto)
{
    int ret;
    __asm__ __volatile__("int $0x40"
                         : "=a"(ret)
                         : "a"(KOLIBRI_SYSFN_SOCKET), "b"(KOLIBRI_FN_SOCKET_OPEN), "c"(domain),
                           "d"(type), "S"(proto)
                         : "memory");
    return ret;
}

static int sys_socket_close(int handle)
{
    int ret;
    __asm__ __volatile__("int $0x40"
                         : "=a"(ret)
                         : "a"(KOLIBRI_SYSFN_SOCKET), "b"(KOLIBRI_FN_SOCKET_CLOSE), "c"(handle)
                         : "memory");
    return ret;
}

static int sys_socket_bind(int handle, const void *addr, int addrlen)
{
    int ret;
    __asm__ __volatile__("int $0x40"
                         : "=a"(ret)
                         : "a"(KOLIBRI_SYSFN_SOCKET), "b"(KOLIBRI_FN_SOCKET_BIND), "c"(handle),
                           "d"(addr), "S"(addrlen)
                         : "memory");
    return ret;
}

static int sys_socket_listen(int handle, int backlog)
{
    int ret;
    __asm__ __volatile__("int $0x40"
                         : "=a"(ret)
                         : "a"(KOLIBRI_SYSFN_SOCKET), "b"(KOLIBRI_FN_SOCKET_LISTEN), "c"(handle),
                           "d"(backlog)
                         : "memory");
    return ret;
}

static int sys_socket_connect(int handle, const void *addr, int addrlen)
{
    int ret;
    __asm__ __volatile__("int $0x40"
                         : "=a"(ret)
                         : "a"(KOLIBRI_SYSFN_SOCKET), "b"(KOLIBRI_FN_SOCKET_CONNECT), "c"(handle),
                           "d"(addr), "S"(addrlen)
                         : "memory");
    return ret;
}

static int sys_socket_accept(int handle, void *addr, int *addrlen)
{
    int ret;
    __asm__ __volatile__("int $0x40"
                         : "=a"(ret)
                         : "a"(KOLIBRI_SYSFN_SOCKET), "b"(KOLIBRI_FN_SOCKET_ACCEPT), "c"(handle),
                           "d"(addr), "S"(addrlen)
                         : "memory");
    return ret;
}

static int sys_socket_send(int handle, const void *buf, int len, int flags)
{
    int ret;
    __asm__ __volatile__("int $0x40"
                         : "=a"(ret)
                         : "a"(KOLIBRI_SYSFN_SOCKET), "b"(KOLIBRI_FN_SOCKET_SEND), "c"(handle),
                           "d"(buf), "S"(len), "D"(flags)
                         : "memory");
    return ret;
}

static int sys_socket_recv(int handle, void *buf, int len, int flags)
{
    int ret;
    __asm__ __volatile__("int $0x40"
                         : "=a"(ret)
                         : "a"(KOLIBRI_SYSFN_SOCKET), "b"(KOLIBRI_FN_SOCKET_RECV), "c"(handle),
                           "d"(buf), "S"(len), "D"(flags)
                         : "memory");
    return ret;
}

/* KolibriOS Syscall 5: sleep for given hundredths of a second */
static void sys_sleep_ticks(int ticks)
{
    if (ticks <= 0) ticks = 1;
    __asm__ __volatile__("int $0x40" :: "a"(5), "b"(ticks) : "memory");
}

cais_socket_t *cais_socket_create(cais_socket_type_t type, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    int stype = (type == CAIS_SOCK_UDP) ? KOLIBRI_SOCK_DGRAM : KOLIBRI_SOCK_STREAM;
    int proto = (type == CAIS_SOCK_UDP) ? KOLIBRI_IPPROTO_UDP : KOLIBRI_IPPROTO_TCP;
    int handle;
    cais_socket_t *sock;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    handle = sys_socket_open(KOLIBRI_AF_INET4, stype, proto);
    if (handle < 0) {
        cais_set_err(err_buf, err_cap, "KolibriOS: failed to create socket (network stack running?)");
        return NULL;
    }

    sock = (cais_socket_t *)malloc(sizeof(cais_socket_t));
    if (sock == NULL) {
        sys_socket_close(handle);
        cais_set_err(err_buf, err_cap, "out of memory");
        return NULL;
    }

    sock->fd = handle;
    sock->type = type;
    sock->mode = mode;
    sock->timeout_ms = 0;
    return sock;
}

void cais_socket_backend_close(cais_socket_t *sock)
{
    if (sock != NULL && sock->fd >= 0) {
        sys_socket_close(sock->fd);
        sock->fd = -1;
    }
}

cais_socket_status_t cais_socket_bind_sock(cais_socket_t *sock, const char *host, int port, int backlog, char *err_buf, size_t err_cap)
{
    unsigned long ip;
    struct kolibri_sockaddr_in addr;
    int rc;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!CAIS_SOCK_IS_VALID(sock)) {
        cais_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }

    if (!cais_socket_parse_ipv4(cais_socket_normalize_bind_host(host), &ip)) {
        cais_set_err(err_buf, err_cap, "KolibriOS: invalid bind address");
        return CAIS_SOCK_ERROR;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = KOLIBRI_AF_INET4;
    addr.sin_port = kolibri_htons((uint16_t)port);
    addr.sin_addr = (uint32_t)ip;

    rc = sys_socket_bind(sock->fd, &addr, sizeof(addr));
    if (rc != 0) {
        cais_set_err(err_buf, err_cap, "KolibriOS: failed to bind socket");
        return CAIS_SOCK_ERROR;
    }

    if (sock->type == CAIS_SOCK_TCP) {
        rc = sys_socket_listen(sock->fd, backlog > 0 ? backlog : 5);
        if (rc != 0) {
            cais_set_err(err_buf, err_cap, "KolibriOS: failed to listen on socket");
            return CAIS_SOCK_ERROR;
        }
    }

    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_connect_sock(cais_socket_t *sock, const char *host, int port, char *err_buf, size_t err_cap)
{
    unsigned long ip;
    struct kolibri_sockaddr_in addr;
    int rc;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!CAIS_SOCK_IS_VALID(sock)) {
        cais_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }

    if (!cais_socket_parse_ipv4(cais_socket_normalize_connect_host(host), &ip)) {
        cais_set_err(err_buf, err_cap, "KolibriOS: invalid IPv4 address format");
        return CAIS_SOCK_ERROR;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = KOLIBRI_AF_INET4;
    addr.sin_port = kolibri_htons((uint16_t)port);
    addr.sin_addr = (uint32_t)ip;

    rc = sys_socket_connect(sock->fd, &addr, sizeof(addr));
    if (rc != 0) {
        if (sock->mode == CAIS_SOCK_NONBLOCKING && (rc == -11 || rc == -1)) {
            return CAIS_SOCK_AGAIN;
        }
        cais_set_err(err_buf, err_cap, "KolibriOS: failed to connect to remote host");
        return CAIS_SOCK_ERROR;
    }

    return CAIS_SOCK_OK;
}

cais_socket_t *cais_socket_accept(cais_socket_t *server, char *client_ip, size_t ip_len,
                                  int *client_port, char *err_buf, size_t err_cap)
{
    struct kolibri_sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    int client_handle;
    cais_socket_t *sock;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!CAIS_SOCK_IS_VALID(server)) {
        cais_set_err(err_buf, err_cap, "invalid server socket");
        return NULL;
    }

    memset(&client_addr, 0, sizeof(client_addr));
    client_handle = sys_socket_accept(server->fd, &client_addr, &addr_len);
    if (client_handle < 0) {
        if (server->mode == CAIS_SOCK_NONBLOCKING && (client_handle == -11 || client_handle == -1)) {
            if (err_buf != NULL && err_cap > 0) {
                err_buf[0] = '\0';
            }
            return NULL;
        }
        cais_set_err(err_buf, err_cap, "KolibriOS: accept failed");
        return NULL;
    }

    if (client_ip != NULL && ip_len > 0) {
        uint32_t ip = kolibri_ntohl(client_addr.sin_addr);
        snprintf(client_ip, ip_len, "%u.%u.%u.%u", (unsigned int)((ip >> 24) & 0xFF),
                 (unsigned int)((ip >> 16) & 0xFF), (unsigned int)((ip >> 8) & 0xFF),
                 (unsigned int)(ip & 0xFF));
    }

    if (client_port != NULL) {
        *client_port = (int)kolibri_ntohs(client_addr.sin_port);
    }

    sock = (cais_socket_t *)malloc(sizeof(cais_socket_t));
    if (sock == NULL) {
        sys_socket_close(client_handle);
        cais_set_err(err_buf, err_cap, "out of memory");
        return NULL;
    }

    sock->fd = client_handle;
    sock->type = CAIS_SOCK_TCP;
    sock->mode = server->mode;
    sock->timeout_ms = server->timeout_ms;
    return sock;
}

cais_socket_status_t cais_socket_backend_send(cais_socket_t *sock, const char *buf, size_t len, size_t *sent)
{
    int flags = 0;
    int ret;
    if (sent != NULL) {
        *sent = 0;
    }
    if (!CAIS_SOCK_IS_VALID(sock) || buf == NULL) {
        return CAIS_SOCK_ERROR;
    }

    if (sock->mode == CAIS_SOCK_NONBLOCKING) {
        flags = KOLIBRI_MSG_DONTWAIT;
    }

    ret = sys_socket_send(sock->fd, buf, (int)len, flags);
    if (ret < 0) {
        if (ret == -11 || (sock->mode == CAIS_SOCK_NONBLOCKING && ret == -1)) {
            return CAIS_SOCK_AGAIN;
        }
        return CAIS_SOCK_ERROR;
    }
    if (sent != NULL) {
        *sent = (size_t)ret;
    }
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_recv(cais_socket_t *sock, char *buf, size_t max_len, size_t *received)
{
    int flags = 0;
    int ret;
    if (received != NULL) {
        *received = 0;
    }
    if (!CAIS_SOCK_IS_VALID(sock) || buf == NULL || max_len == 0) {
        return CAIS_SOCK_ERROR;
    }

    if (sock->mode == CAIS_SOCK_NONBLOCKING) {
        flags = KOLIBRI_MSG_DONTWAIT;
    }

    ret = sys_socket_recv(sock->fd, buf, (int)max_len, flags);
    if (ret < 0) {
        if (ret == -11 || (sock->mode == CAIS_SOCK_NONBLOCKING && ret == -1)) {
            return CAIS_SOCK_AGAIN;
        }
        return CAIS_SOCK_ERROR;
    }
    if (ret == 0 && sock->type == CAIS_SOCK_TCP) {
        return CAIS_SOCK_CLOSED;
    }
    if (received != NULL) {
        *received = (size_t)ret;
    }
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_sendto(cais_socket_t *sock, const char *buf, size_t len,
                                        const char *host, int port, size_t *sent)
{
    unsigned long ip;
    struct kolibri_sockaddr_in addr;

    if (sent != NULL) {
        *sent = 0;
    }
    if (!CAIS_SOCK_IS_VALID(sock) || buf == NULL || host == NULL) {
        return CAIS_SOCK_ERROR;
    }

    if (!cais_socket_parse_ipv4(cais_socket_normalize_connect_host(host), &ip)) {
        return CAIS_SOCK_ERROR;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = KOLIBRI_AF_INET4;
    addr.sin_port = kolibri_htons((uint16_t)port);
    addr.sin_addr = (uint32_t)ip;

    sys_socket_connect(sock->fd, &addr, sizeof(addr));
    return cais_socket_send(sock, buf, len, sent);
}

cais_socket_status_t cais_socket_recvfrom(cais_socket_t *sock, char *buf, size_t max_len,
                                          char *src_ip, size_t ip_len, int *src_port,
                                          size_t *received)
{
    cais_socket_status_t status = cais_socket_recv(sock, buf, max_len, received);
    if (status == CAIS_SOCK_OK) {
        if (src_ip != NULL && ip_len > 0) {
            src_ip[0] = '\0';
        }
        if (src_port != NULL) {
            *src_port = 0;
        }
    }
    return status;
}

cais_socket_status_t cais_socket_backend_set_mode(cais_socket_t *sock, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!CAIS_SOCK_IS_VALID(sock)) {
        cais_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }

    sock->mode = mode;
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_backend_set_timeout(cais_socket_t *sock, int timeout_ms, char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!CAIS_SOCK_IS_VALID(sock)) {
        cais_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }

    sock->timeout_ms = cais_socket_normalize_timeout(timeout_ms);
    return CAIS_SOCK_OK;
}

int cais_socket_backend_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms)
{
    int elapsed = 0;
    int step_ms = 10;
    size_t i;

    if (fds == NULL && nfds > 0) return CAIS_SOCK_ERROR;

    for (i = 0; i < nfds; i++) {
        fds[i].revents = 0;
    }

    while (timeout_ms < 0 || elapsed <= timeout_ms) {
        int count = 0;
        for (i = 0; i < nfds; i++) {
            if (fds[i].sock != NULL && fds[i].sock->fd >= 0) {
                int revents = 0;
                if (fds[i].events & CAIS_SOCK_POLL_READ) {
                    char peek;
                    int n = sys_socket_recv(fds[i].sock->fd, &peek, 1, KOLIBRI_MSG_DONTWAIT);
                    if (n >= 0) {
                        revents |= CAIS_SOCK_POLL_READ;
                    }
                }
                if (fds[i].events & CAIS_SOCK_POLL_WRITE) {
                    revents |= CAIS_SOCK_POLL_WRITE;
                }
                if (revents != 0) {
                    fds[i].revents = revents;
                    count++;
                }
            }
        }

        if (count > 0) {
            return count;
        }

        if (timeout_ms >= 0 && elapsed >= timeout_ms) {
            break;
        }

        sys_sleep_ticks(1); /* 10 ms sleep */
        elapsed += step_ms;
    }

    return 0;
}

#else
typedef int make_iso_compilers_happy_socket_kolibri;
#endif /* CAIS_IS_KOLIBRI */
