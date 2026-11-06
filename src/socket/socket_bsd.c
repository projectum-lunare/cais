/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "socket_pal.h"
#include "socket_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CAIS_SOCKLEN_T
#if CAIS_IS_WINDOWS || CAIS_IS_OS2 || CAIS_IS_DOS
#define CAIS_SOCKLEN_T int
#elif CAIS_IS_AMIGA
#define CAIS_SOCKLEN_T unsigned long
#else
#define CAIS_SOCKLEN_T socklen_t
#endif
#endif

/*
 * Resolves an IPv4 address/hostname and port into a struct sockaddr_in.
 */
static int cais_pal_resolve_ipv4(const char *host, int port, struct sockaddr_in *sin, char *err_buf, size_t err_cap)
{
    unsigned long ip_be = 0;

    memset(sin, 0, sizeof(*sin));
    sin->sin_family = AF_INET;
    sin->sin_port = htons((unsigned short)port);

    if (host == NULL || host[0] == '\0' || strcmp(host, "0.0.0.0") == 0 || strcmp(host, "*") == 0) {
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
        return 0;
    }

    if (strcmp(host, "255.255.255.255") == 0) {
        sin->sin_addr.s_addr = 0xFFFFFFFFUL;
        return 0;
    }

    if (cais_socket_parse_ipv4(host, &ip_be)) {
        sin->sin_addr.s_addr = htonl(ip_be);
        return 0;
    }

    /* Hostname lookup via DNS / hosts */
    {
        struct hostent *he = gethostbyname(host);
        if (he != NULL && he->h_addr_list != NULL && he->h_addr_list[0] != NULL) {
            memcpy(&sin->sin_addr, he->h_addr_list[0], sizeof(sin->sin_addr));
            return 0;
        }
    }

    cais_set_err_fmt(err_buf, err_cap, "cannot resolve host '%.128s'", host);
    return -1;
}

cais_socket_t *cais_socket_create(cais_socket_type_t type, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    int socktype = (type == CAIS_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    CAIS_PAL_SOCK_T fd;
    cais_socket_t *sock;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (!cais_pal_sock_init(err_buf, err_cap)) {
        return NULL;
    }

    fd = socket(AF_INET, socktype, 0);
    if (fd == CAIS_PAL_INVALID_SOCK) {
        cais_pal_strerror(CAIS_PAL_SOCK_ERRNO, err_buf, err_cap);
        return NULL;
    }

    if (mode == CAIS_SOCK_NONBLOCKING) {
        if (cais_pal_set_nonblocking(fd, 1) != 0) {
            cais_pal_strerror(CAIS_PAL_SOCK_ERRNO, err_buf, err_cap);
            CAIS_PAL_CLOSESOCKET(fd);
            return NULL;
        }
    }

    sock = (cais_socket_t *)malloc(sizeof(cais_socket_t));
    if (sock == NULL) {
        CAIS_PAL_CLOSESOCKET(fd);
        cais_set_err(err_buf, err_cap, "out of memory allocating socket structure");
        return NULL;
    }

    sock->fd = fd;
    sock->type = type;
    sock->mode = mode;
    sock->timeout_ms = 0;
    return sock;
}

void cais_socket_backend_close(cais_socket_t *sock)
{
    if (sock != NULL && CAIS_SOCK_IS_VALID(sock)) {
        CAIS_PAL_CLOSESOCKET(sock->fd);
        sock->fd = CAIS_PAL_INVALID_SOCK;
    }
}

cais_socket_status_t cais_socket_bind_sock(cais_socket_t *sock, const char *host, int port, int backlog, char *err_buf, size_t err_cap)
{
    struct sockaddr_in sin;
    int opt = 1;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (sock == NULL || !CAIS_SOCK_IS_VALID(sock)) {
        cais_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }

    host = cais_socket_normalize_bind_host(host);
    if (cais_pal_resolve_ipv4(host, port, &sin, err_buf, err_cap) != 0) {
        return CAIS_SOCK_ERROR;
    }

    setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    if (bind(sock->fd, (struct sockaddr *)&sin, (CAIS_SOCKLEN_T)sizeof(sin)) != 0) {
        cais_pal_strerror(CAIS_PAL_SOCK_ERRNO, err_buf, err_cap);
        return CAIS_SOCK_ERROR;
    }

    if (sock->type == CAIS_SOCK_TCP) {
        if (listen(sock->fd, backlog > 0 ? backlog : 128) != 0) {
            cais_pal_strerror(CAIS_PAL_SOCK_ERRNO, err_buf, err_cap);
            return CAIS_SOCK_ERROR;
        }
    }

    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_connect_sock(cais_socket_t *sock, const char *host, int port, char *err_buf, size_t err_cap)
{
    struct sockaddr_in sin;
    int res;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (sock == NULL || !CAIS_SOCK_IS_VALID(sock)) {
        cais_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }

    host = cais_socket_normalize_connect_host(host);
    if (cais_pal_resolve_ipv4(host, port, &sin, err_buf, err_cap) != 0) {
        return CAIS_SOCK_ERROR;
    }

    res = connect(sock->fd, (struct sockaddr *)&sin, (CAIS_SOCKLEN_T)sizeof(sin));
    if (res != 0) {
        int err = CAIS_PAL_SOCK_ERRNO;
        if (CAIS_PAL_IS_INPROGRESS(err) || CAIS_PAL_IS_WOULDBLOCK(err)) {
            return CAIS_SOCK_AGAIN;
        }
        cais_pal_strerror(err, err_buf, err_cap);
        return CAIS_SOCK_ERROR;
    }

    return CAIS_SOCK_OK;
}

cais_socket_t *cais_socket_accept(cais_socket_t *server, char *client_ip, size_t ip_len,
                                  int *client_port, char *err_buf, size_t err_cap)
{
    struct sockaddr_in addr;
    CAIS_SOCKLEN_T addr_len = (CAIS_SOCKLEN_T)sizeof(addr);
    CAIS_PAL_SOCK_T client_fd;
    cais_socket_t *client_sock;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (server == NULL || !CAIS_SOCK_IS_VALID(server)) {
        cais_set_err(err_buf, err_cap, "invalid server socket");
        return NULL;
    }

    client_fd = accept(server->fd, (struct sockaddr *)&addr, &addr_len);
    if (client_fd == CAIS_PAL_INVALID_SOCK) {
        int err = CAIS_PAL_SOCK_ERRNO;
        if (CAIS_PAL_IS_WOULDBLOCK(err)) {
            return NULL;
        }
        cais_pal_strerror(err, err_buf, err_cap);
        return NULL;
    }

    if (client_ip != NULL && ip_len > 0) {
        unsigned long ip_h = ntohl(addr.sin_addr.s_addr);
        cais_socket_format_ipv4(ip_h, client_ip, ip_len);
        if (client_port != NULL) {
            *client_port = (int)ntohs(addr.sin_port);
        }
    }

    client_sock = (cais_socket_t *)malloc(sizeof(cais_socket_t));
    if (client_sock == NULL) {
        CAIS_PAL_CLOSESOCKET(client_fd);
        cais_set_err(err_buf, err_cap, "out of memory allocating client socket");
        return NULL;
    }

    client_sock->fd = client_fd;
    client_sock->type = CAIS_SOCK_TCP;
    client_sock->mode = server->mode;
    client_sock->timeout_ms = 0;

    if (server->mode == CAIS_SOCK_NONBLOCKING) {
        cais_pal_set_nonblocking(client_fd, 1);
    }

    return client_sock;
}

cais_socket_status_t cais_socket_backend_send(cais_socket_t *sock, const char *buf, size_t len, size_t *sent)
{
    int n;
    if (sent != NULL) {
        *sent = 0;
    }
    if (sock == NULL || !CAIS_SOCK_IS_VALID(sock) || buf == NULL) {
        return CAIS_SOCK_ERROR;
    }

#if defined(MSG_NOSIGNAL)
    n = send(sock->fd, buf, (int)len, MSG_NOSIGNAL);
#else
    n = send(sock->fd, buf, (int)len, 0);
#endif

    if (n < 0) {
        int err = CAIS_PAL_SOCK_ERRNO;
        if (CAIS_PAL_IS_INTR(err) || CAIS_PAL_IS_WOULDBLOCK(err)) {
            return CAIS_SOCK_AGAIN;
        }
        return CAIS_SOCK_ERROR;
    }

    if (sent != NULL) {
        *sent = (size_t)n;
    }
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_recv(cais_socket_t *sock, char *buf, size_t max_len, size_t *received)
{
    int n;
    if (received != NULL) {
        *received = 0;
    }
    if (sock == NULL || !CAIS_SOCK_IS_VALID(sock) || buf == NULL || max_len == 0) {
        return CAIS_SOCK_ERROR;
    }

    while (1) {
        n = recv(sock->fd, buf, (int)max_len, 0);
        if (n < 0) {
            int err = CAIS_PAL_SOCK_ERRNO;
            if (CAIS_PAL_IS_INTR(err)) {
                continue;
            }
            if (CAIS_PAL_IS_WOULDBLOCK(err)) {
                return CAIS_SOCK_AGAIN;
            }
            return CAIS_SOCK_ERROR;
        }
        break;
    }

    if (n == 0 && sock->type == CAIS_SOCK_TCP) {
        return CAIS_SOCK_CLOSED;
    }

    if (received != NULL) {
        *received = (size_t)n;
    }
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_sendto(cais_socket_t *sock, const char *buf, size_t len,
                                        const char *host, int port, size_t *sent)
{
    struct sockaddr_in sin;
    int n;

    if (sent != NULL) {
        *sent = 0;
    }
    if (sock == NULL || !CAIS_SOCK_IS_VALID(sock) || buf == NULL) {
        return CAIS_SOCK_ERROR;
    }

    host = cais_socket_normalize_connect_host(host);
    if (cais_pal_resolve_ipv4(host, port, &sin, NULL, 0) != 0) {
        return CAIS_SOCK_ERROR;
    }

    while (1) {
        n = sendto(sock->fd, buf, (int)len, 0, (struct sockaddr *)&sin, (CAIS_SOCKLEN_T)sizeof(sin));
        if (n < 0) {
            int err = CAIS_PAL_SOCK_ERRNO;
            if (CAIS_PAL_IS_INTR(err)) {
                continue;
            }
            if (CAIS_PAL_IS_WOULDBLOCK(err)) {
                return CAIS_SOCK_AGAIN;
            }
            return CAIS_SOCK_ERROR;
        }
        break;
    }

    if (sent != NULL) {
        *sent = (size_t)n;
    }
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_recvfrom(cais_socket_t *sock, char *buf, size_t max_len,
                                          char *src_ip, size_t ip_len, int *src_port,
                                          size_t *received)
{
    struct sockaddr_in sin;
    CAIS_SOCKLEN_T addr_len = (CAIS_SOCKLEN_T)sizeof(sin);
    int n;

    if (received != NULL) {
        *received = 0;
    }
    if (sock == NULL || !CAIS_SOCK_IS_VALID(sock) || buf == NULL || max_len == 0) {
        return CAIS_SOCK_ERROR;
    }

    while (1) {
        n = recvfrom(sock->fd, buf, (int)max_len, 0, (struct sockaddr *)&sin, &addr_len);
        if (n < 0) {
            int err = CAIS_PAL_SOCK_ERRNO;
            if (CAIS_PAL_IS_INTR(err)) {
                continue;
            }
            if (CAIS_PAL_IS_WOULDBLOCK(err)) {
                return CAIS_SOCK_AGAIN;
            }
            return CAIS_SOCK_ERROR;
        }
        break;
    }

    if (n == 0 && sock->type == CAIS_SOCK_TCP) {
        return CAIS_SOCK_CLOSED;
    }

    if (src_ip != NULL && ip_len > 0) {
        unsigned long ip_h = ntohl(sin.sin_addr.s_addr);
        cais_socket_format_ipv4(ip_h, src_ip, ip_len);
        if (src_port != NULL) {
            *src_port = (int)ntohs(sin.sin_port);
        }
    }

    if (received != NULL) {
        *received = (size_t)n;
    }
    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_backend_set_mode(cais_socket_t *sock, cais_socket_mode_t mode, char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (sock == NULL || !CAIS_SOCK_IS_VALID(sock)) {
        cais_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }

    if (cais_pal_set_nonblocking(sock->fd, (mode == CAIS_SOCK_NONBLOCKING) ? 1 : 0) != 0) {
        cais_pal_strerror(CAIS_PAL_SOCK_ERRNO, err_buf, err_cap);
        return CAIS_SOCK_ERROR;
    }

    return CAIS_SOCK_OK;
}

cais_socket_status_t cais_socket_backend_set_timeout(cais_socket_t *sock, int timeout_ms, char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (sock == NULL || !CAIS_SOCK_IS_VALID(sock)) {
        cais_set_err(err_buf, err_cap, "invalid socket handle");
        return CAIS_SOCK_ERROR;
    }

    if (cais_pal_set_timeout(sock->fd, timeout_ms) != 0) {
        cais_pal_strerror(CAIS_PAL_SOCK_ERRNO, err_buf, err_cap);
        return CAIS_SOCK_ERROR;
    }

    return CAIS_SOCK_OK;
}

/* =============================================================================
 * Polling & Event Multiplexing Engines
 * ============================================================================= */

/*
 * -----------------------------------------------------------------------------
 * 1. Universal select() Fallback Engine
 * -----------------------------------------------------------------------------
 * Used as:
 * - Native multiplexer on vintage Non-POSIX BSD stacks:
 *     - MS-DOS (WATT-32 select_s)
 *     - AmigaOS (bsdsocket.library WaitSelect / select)
 *     - OS/2 / ArcaOS (IBM TCP/IP os2_select)
 *     - Atari ST (MiNTNet / TOS BSD socketlib)
 * - Transparent fallback on Windows NT 3.51 / NT 4.0 / 2000 / XP when WSAPoll
 *   is not present in WS2_32.DLL.
 *
 * NOTE ON WIN32 FD_SETSIZE:
 * On Winsock, fd_set is an array of SOCKET handles with an explicit counter,
 * NOT a bitmask. By redefining FD_SETSIZE to 1024 before <winsock2.h>, this
 * fallback safely scales up to 1024 concurrent sockets even on Windows NT 3.51
 * without stack buffer overflow.
 */
#if !CAIS_IS_POSIX || defined(CAIS_FORCE_SELECT)
static int cais_poll_via_select(cais_pollfd_t *fds, size_t nfds, int timeout_ms)
{
    fd_set read_fds, write_fds, err_fds;
    CAIS_PAL_SOCK_T max_fd = 0;
    struct timeval tv;
    struct timeval *ptv = NULL;
    size_t i;
    int ret;
    int ready_count = 0;

    if (fds == NULL && nfds > 0) {
        return CAIS_SOCK_ERROR;
    }
    if (nfds == 0) {
        if (timeout_ms > 0) {
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000L;
            CAIS_PAL_SELECT(0, NULL, NULL, NULL, &tv);
        }
        return 0;
    }

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&err_fds);

    for (i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if (fds[i].sock != NULL && CAIS_SOCK_IS_VALID(fds[i].sock)) {
            CAIS_PAL_SOCK_T s = fds[i].sock->fd;
            if (fds[i].events & CAIS_SOCK_POLL_READ) {
                FD_SET(s, &read_fds);
            }
            if (fds[i].events & CAIS_SOCK_POLL_WRITE) {
                FD_SET(s, &write_fds);
            }
            FD_SET(s, &err_fds);
            if (s > max_fd) {
                max_fd = s;
            }
        }
    }

    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000L;
        ptv = &tv;
    }

    while (1) {
        ret = CAIS_PAL_SELECT(max_fd + 1, &read_fds, &write_fds, &err_fds, ptv);
        if (ret < 0) {
            int err = CAIS_PAL_SOCK_ERRNO;
            if (CAIS_PAL_IS_INTR(err)) {
                continue;
            }
            return CAIS_SOCK_ERROR;
        }
        break;
    }

    if (ret > 0) {
        for (i = 0; i < nfds; i++) {
            if (fds[i].sock != NULL && CAIS_SOCK_IS_VALID(fds[i].sock)) {
                CAIS_PAL_SOCK_T s = fds[i].sock->fd;
                if (FD_ISSET(s, &read_fds)) {
                    fds[i].revents |= CAIS_SOCK_POLL_READ;
                }
                if (FD_ISSET(s, &write_fds)) {
                    fds[i].revents |= CAIS_SOCK_POLL_WRITE;
                }
                if (FD_ISSET(s, &err_fds)) {
                    fds[i].revents |= CAIS_SOCK_POLL_ERR;
                }
                if (fds[i].revents != 0) {
                    ready_count++;
                }
            }
        }
    }

    return ready_count;
}
#endif

#if CAIS_IS_POSIX
/*
 * -----------------------------------------------------------------------------
 * 2. Modern POSIX poll(2) Engine
 * -----------------------------------------------------------------------------
 * Native implementation for POSIX.1-2001 operating systems (Linux, FreeBSD,
 * OpenBSD, NetBSD, macOS / Darwin, Haiku, Solaris).
 *
 * ADVANTAGES OVER select():
 * 1. Bypasses the fixed FD_SETSIZE ceiling: file descriptor IDs >= 1024 are
 *    handled transparently without risking stack buffer corruption.
 * 2. Linear O(nfds) array scanning without re-zeroing bitmasks on every tick.
 * 3. Stack allocation optimization for small sets (<= 64 fds) to eliminate
 *    dynamic memory allocation overhead on fast paths.
 */
static int cais_posix_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms)
{
    struct pollfd stack_pfds[64];
    struct pollfd *pfds = stack_pfds;
    size_t i;
    int ret;
    int ready_count = 0;

    if (fds == NULL && nfds > 0) {
        return CAIS_SOCK_ERROR;
    }
    if (nfds == 0) {
        if (timeout_ms > 0) {
            poll(NULL, 0, timeout_ms);
        }
        return 0;
    }

    if (nfds > sizeof(stack_pfds) / sizeof(stack_pfds[0])) {
        pfds = (struct pollfd *)malloc(nfds * sizeof(struct pollfd));
        if (pfds == NULL) {
            return CAIS_SOCK_ERROR;
        }
    }

    for (i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if (fds[i].sock != NULL && CAIS_SOCK_IS_VALID(fds[i].sock)) {
            pfds[i].fd = fds[i].sock->fd;
            pfds[i].events = 0;
            if (fds[i].events & CAIS_SOCK_POLL_READ) {
                pfds[i].events |= POLLIN;
            }
            if (fds[i].events & CAIS_SOCK_POLL_WRITE) {
                pfds[i].events |= POLLOUT;
            }
        } else {
            /* POSIX poll() ignores entries with negative fd and sets revents = 0 */
            pfds[i].fd = -1;
            pfds[i].events = 0;
        }
        pfds[i].revents = 0;
    }

    while (1) {
        ret = poll(pfds, (nfds_t)nfds, timeout_ms);
        if (ret < 0) {
            if (CAIS_PAL_IS_INTR(errno)) {
                continue;
            }
            if (pfds != stack_pfds) {
                free(pfds);
            }
            return CAIS_SOCK_ERROR;
        }
        break;
    }

    if (ret > 0) {
        for (i = 0; i < nfds; i++) {
            if (pfds[i].fd >= 0 && pfds[i].revents != 0) {
                if (pfds[i].revents & (POLLIN | POLLPRI)) {
                    fds[i].revents |= CAIS_SOCK_POLL_READ;
                }
                if (pfds[i].revents & POLLOUT) {
                    fds[i].revents |= CAIS_SOCK_POLL_WRITE;
                }
                if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    fds[i].revents |= CAIS_SOCK_POLL_ERR;
                }
                if (fds[i].revents != 0) {
                    ready_count++;
                }
            }
        }
    }

    if (pfds != stack_pfds) {
        free(pfds);
    }
    return ready_count;
}

#elif CAIS_IS_WINDOWS
#if CAIS_IS_WINDOWS_CE
static int cais_win32_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms)
{
    return cais_poll_via_select(fds, nfds, timeout_ms);
}
#else
/*
 * -----------------------------------------------------------------------------
 * 3. Windows Adaptive WSAPoll / select() Engine
 * -----------------------------------------------------------------------------
 * Seamlessly bridges Windows NT 3.51 (1995) to modern Windows 11 (2026).
 *
 * CRITICAL VINTAGE NT COMPATIBILITY:
 * WSAPoll was introduced in Windows Vista (NT 6.0). Calling WSAPoll statically
 * would bake an import table reference to WS2_32.DLL that prevents the PE
 * loader on Windows NT 3.51, NT 4.0, 2000, and XP from even starting the executable!
 *
 * SOLUTION:
 * We resolve WSAPoll dynamically via GetProcAddress:
 * - On Windows Vista..11 / Server 2008..2025: WSAPoll is resolved and invoked.
 * - On Windows NT 3.51..XP: GetProcAddress returns NULL and we fall back to
 *   cais_poll_via_select(), with FD_SETSIZE expanded to 1024.
 */

#define CAIS_WSA_POLLRDNORM 0x0100
#define CAIS_WSA_POLLRDBAND 0x0200
#define CAIS_WSA_POLLIN     (CAIS_WSA_POLLRDNORM | CAIS_WSA_POLLRDBAND)
#define CAIS_WSA_POLLPRI    0x0400
#define CAIS_WSA_POLLWRNORM 0x0010
#define CAIS_WSA_POLLOUT    (CAIS_WSA_POLLWRNORM)
#define CAIS_WSA_POLLERR    0x0001
#define CAIS_WSA_POLLHUP    0x0002
#define CAIS_WSA_POLLNVAL   0x0004

/* Exact memory ABI structure matching Winsock2 WSAPOLLFD across x86/x64/ARM */
typedef struct cais_wsapollfd {
    SOCKET fd;
    short  events;
    short  revents;
} cais_wsapollfd_t;

typedef int (WSAAPI *cais_pfn_wsapoll_t)(cais_wsapollfd_t *fdArray, ULONG fds, INT timeout);

static cais_pfn_wsapoll_t s_pfn_WSAPoll = NULL;
static int s_wsapoll_probed = 0;

static void cais_win32_probe_wsapoll(void)
{
    if (!s_wsapoll_probed) {
        HMODULE hws2 = GetModuleHandleA("ws2_32.dll");
        if (hws2 != NULL) {
            s_pfn_WSAPoll = (cais_pfn_wsapoll_t)GetProcAddress(hws2, "WSAPoll");
        }
        s_wsapoll_probed = 1;
    }
}

static int cais_win32_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms)
{
    cais_wsapollfd_t stack_pfds[64];
    cais_wsapollfd_t *pfds = stack_pfds;
    size_t i;
    int ret;
    int ready_count = 0;

    cais_win32_probe_wsapoll();

    /* If WSAPoll is unavailable (WinNT 3.51 / 4.0 / 2000 / XP), use select() */
    if (s_pfn_WSAPoll == NULL) {
        return cais_poll_via_select(fds, nfds, timeout_ms);
    }

    if (fds == NULL && nfds > 0) {
        return CAIS_SOCK_ERROR;
    }
    if (nfds == 0) {
        if (timeout_ms > 0) {
            Sleep((DWORD)timeout_ms);
        }
        return 0;
    }

    if (nfds > sizeof(stack_pfds) / sizeof(stack_pfds[0])) {
        pfds = (cais_wsapollfd_t *)malloc(nfds * sizeof(cais_wsapollfd_t));
        if (pfds == NULL) {
            return CAIS_SOCK_ERROR;
        }
    }

    for (i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if (fds[i].sock != NULL && CAIS_SOCK_IS_VALID(fds[i].sock)) {
            pfds[i].fd = fds[i].sock->fd;
            pfds[i].events = 0;
            if (fds[i].events & CAIS_SOCK_POLL_READ) {
                pfds[i].events |= CAIS_WSA_POLLIN;
            }
            if (fds[i].events & CAIS_SOCK_POLL_WRITE) {
                pfds[i].events |= CAIS_WSA_POLLOUT;
            }
        } else {
            pfds[i].fd = INVALID_SOCKET;
            pfds[i].events = 0;
        }
        pfds[i].revents = 0;
    }

    while (1) {
        ret = s_pfn_WSAPoll(pfds, (ULONG)nfds, timeout_ms);
        if (ret < 0) {
            int err = WSAGetLastError();
            if (CAIS_PAL_IS_INTR(err)) {
                continue;
            }
            if (pfds != stack_pfds) {
                free(pfds);
            }
            return CAIS_SOCK_ERROR;
        }
        break;
    }

    if (ret > 0) {
        for (i = 0; i < nfds; i++) {
            if (pfds[i].fd != INVALID_SOCKET && pfds[i].revents != 0) {
                if (pfds[i].revents & (CAIS_WSA_POLLIN | CAIS_WSA_POLLPRI)) {
                    fds[i].revents |= CAIS_SOCK_POLL_READ;
                }
                if (pfds[i].revents & CAIS_WSA_POLLOUT) {
                    fds[i].revents |= CAIS_SOCK_POLL_WRITE;
                }
                if (pfds[i].revents & (CAIS_WSA_POLLERR | CAIS_WSA_POLLHUP | CAIS_WSA_POLLNVAL)) {
                    fds[i].revents |= CAIS_SOCK_POLL_ERR;
                }
                if (fds[i].revents != 0) {
                    ready_count++;
                }
            }
        }
    }

    if (pfds != stack_pfds) {
        free(pfds);
    }
    return ready_count;
}
#endif /* !CAIS_IS_WINDOWS_CE */
#endif

int cais_socket_backend_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms)
{
#if defined(CAIS_PAL_HAS_CUSTOM_POLL)
    return cais_pal_custom_poll(fds, nfds, timeout_ms);
#elif CAIS_IS_WINDOWS
    return cais_win32_poll(fds, nfds, timeout_ms);
#elif CAIS_IS_POSIX
    return cais_posix_poll(fds, nfds, timeout_ms);
#elif CAIS_IS_DOS || CAIS_IS_AMIGA || CAIS_IS_OS2 || CAIS_IS_ATARIST
    return cais_poll_via_select(fds, nfds, timeout_ms);
#else
    return cais_poll_via_select(fds, nfds, timeout_ms);
#endif
}
