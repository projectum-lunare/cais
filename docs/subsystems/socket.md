# Network Sockets & Multiplexing (`cais.socket`)

The `socket` subsystem provides a cross-platform, non-blocking IPv4 TCP (streaming) and UDP (datagram) engine designed to maintain 100% semantic parity between modern POSIX workstations and vintage operating systems.

It abstracts socket creation, address resolution, connection establishment, datagram routing, timeout enforcement, and event multiplexing (`cais_socket_poll` / `sock.select`) across Linux, macOS, BSDs, Windows NT, Windows CE, and retro TCP/IP stacks.

---

## ⚡ 1. Architectural Principles & Guarantees

### 1.1 Complete POSIX vs Windows Semantic Parity
Historically, network programming differs radically between POSIX and Windows:
* Windows requires initializing and terminating Winsock via `WSAStartup()` and `WSACleanup()`.
* Windows uses `SOCKET` (unsigned integer pointer) and `closesocket()`, while POSIX uses integer file descriptors and `close()`.
* Non-blocking configuration requires `ioctl(fd, FIONBIO)` or `fcntl(fd, F_SETFL, O_NONBLOCK)`.
* Errors return through `errno` on POSIX, but through `WSAGetLastError()` on Windows.

`cais.socket` **eradicates all these discrepancies**. The application uses identical function calls, identical status codes (`CAIS_SOCK_AGAIN`, `CAIS_SOCK_CLOSED`), and identical error reporting everywhere.

---

### 1.2 Immunity to `FD_SETSIZE` (1024 Descriptors) Stack Corruption
On POSIX, classic `select()` relies on `fd_set`, a fixed-size stack bitmask bounded by `FD_SETSIZE` (typically 1024). In server processes with many open files (pipes, databases, logs), socket descriptors exceeding 1023 cause `FD_SET()` to overwrite stack memory, crashing the process.

`cais` solves this permanently:
* On POSIX, `cais_socket_poll()` compiles down to native `poll(2)`, which uses a dynamically sized array of `struct pollfd` with zero stack limitations.
* On modern Windows, it maps to `WSAPoll()`.
* On vintage systems lacking `poll(2)`, it synthesizes a heap-allocated `select()` with boundary validation.

---

### 1.3 Deterministic Non-Blocking State Machine
Sockets can toggle dynamically between blocking (`CAIS_SOCK_BLOCKING`) and non-blocking (`CAIS_SOCK_NONBLOCKING`) modes. In non-blocking mode:
* Read/write operations that cannot complete immediately return `CAIS_SOCK_AGAIN` (`nil, "again"` in Lua).
* Inactive server listeners return `NULL` with `err_buf` empty.
* Applications can multiplex hundreds of concurrent sockets in a single execution thread with zero CPU spinning.

---

## ⚙️ 2. Constants & Enumerations

### 2.1 Socket Protocol Type (`cais_socket_type_t`)

| C Constant                           | Lua Field (`cais.socket.type`) | Value | Description                                    |
| :----------------------------------- | :----------------------------- | :---: | :--------------------------------------------- |
| `CAIS_SOCK_TCP` / `CAIS_SOCK_STREAM` | `TCP`                          |  `1`  | Stream-oriented connection (TCP over IPv4)     |
| `CAIS_SOCK_UDP` / `CAIS_SOCK_DGRAM`  | `UDP`                          |  `2`  | Connectionless datagram socket (UDP over IPv4) |

---

### 2.2 Blocking Mode (`cais_socket_mode_t`)

| C Constant              | Lua Field (`cais.socket.mode`) | Value | Description                                                 |
| :---------------------- | :----------------------------- | :---: | :---------------------------------------------------------- |
| `CAIS_SOCK_BLOCKING`    | `BLOCKING`                     |  `0`  | Operations block execution until completed or timed out     |
| `CAIS_SOCK_NONBLOCKING` | `NONBLOCKING`                  |  `1`  | Operations return immediately; return `AGAIN` if incomplete |

---

### 2.3 Event Polling Bitmasks (`cais_socket_event_t`)

Bitmasks supplied to `cais_socket_poll()` and `sock:wait()`:

| C Constant             | Lua Field (`cais.socket.event`) |      Bitmask      | Description                                           |
| :--------------------- | :------------------------------ | :---------------: | :---------------------------------------------------- |
| `CAIS_SOCK_POLL_READ`  | `READ`                          | `1 << 0` (`0x01`) | Socket has incoming data or pending client connection |
| `CAIS_SOCK_POLL_WRITE` | `WRITE`                         | `1 << 1` (`0x02`) | Socket has buffer capacity available to transmit data |
| `CAIS_SOCK_POLL_ERR`   | `ERR`                           | `1 << 2` (`0x04`) | Asynchronous error, remote hangup, or broken pipe     |

---

### 2.4 Return Status Codes (`cais_socket_status_t`)

| C Constant          | Lua String  | Value | Description                                                   |
| :------------------ | :---------: | :---: | :------------------------------------------------------------ |
| `CAIS_SOCK_OK`      |   `"ok"`    |  `0`  | Transfer completed successfully                               |
| `CAIS_SOCK_ERROR`   |  `"error"`  | `-1`  | Unrecoverable I/O or network stack error                      |
| `CAIS_SOCK_AGAIN`   |  `"again"`  | `-2`  | Non-blocking operation would block (`EAGAIN` / `EWOULDBLOCK`) |
| `CAIS_SOCK_CLOSED`  | `"closed"`  | `-3`  | Connection orderly closed by remote peer (EOF)                |
| `CAIS_SOCK_TIMEOUT` | `"timeout"` | `-4`  | Configured timeout expired before data could transfer         |

---

## 🏛️ 3. ANSI C89 Function Reference

All functions are declared in [`include/cais/socket.h`](file:///home/gustavo/Projetos/cais/include/cais/socket.h).

### 3.1 Socket Instantiation & Teardown

```c
/* Creates an unbound IPv4 socket */
cais_socket_t *cais_socket_create(
    cais_socket_type_t type, 
    cais_socket_mode_t mode, 
    char *err_buf, 
    size_t err_cap
);

/* Closes socket and frees internal structures */
void cais_socket_close(cais_socket_t *sock);
```

---

### 3.2 TCP Connection & Server Lifecycle

```c
/* Initiates synchronous or non-blocking TCP client connection */
cais_socket_t *cais_socket_connect(
    const char *host, 
    int port, 
    cais_socket_mode_t mode, 
    char *err_buf, 
    size_t err_cap
);

/* Creates, binds, and listens on a TCP server socket */
cais_socket_t *cais_socket_bind(
    const char *host, 
    int port, 
    int backlog, 
    cais_socket_mode_t mode, 
    char *err_buf, 
    size_t err_cap
);

/* Accepts incoming client connection from listening socket */
cais_socket_t *cais_socket_accept(
    cais_socket_t *server, 
    char *client_ip, 
    size_t ip_len, 
    int *client_port, 
    char *err_buf, 
    size_t err_cap
);
```

---

### 3.3 TCP Stream Transmission & Reception

```c
cais_socket_status_t cais_socket_send(
    cais_socket_t *sock, 
    const char *buf, 
    size_t len, 
    size_t *sent
);

cais_socket_status_t cais_socket_recv(
    cais_socket_t *sock, 
    char *buf, 
    size_t max_len, 
    size_t *received
);
```
* **Return Values:**
  * `CAIS_SOCK_OK`: One or more bytes transferred. `*sent` or `*received` populated.
  * `CAIS_SOCK_AGAIN`: Socket operating in non-blocking mode has no data or buffer space.
  * `CAIS_SOCK_CLOSED`: On `recv`, indicates peer shut down the connection cleanly (EOF).
  * `CAIS_SOCK_ERROR`: Fatal socket error.

---

### 3.4 UDP Datagram Routing

```c
cais_socket_status_t cais_socket_sendto(
    cais_socket_t *sock, 
    const char *buf, 
    size_t len,
    const char *host, 
    int port, 
    size_t *sent
);

cais_socket_status_t cais_socket_recvfrom(
    cais_socket_t *sock, 
    char *buf, 
    size_t max_len,
    char *src_ip, 
    size_t ip_len, 
    int *src_port,
    size_t *received
);
```

---

### 3.5 Event Multiplexing (`poll`)

```c
typedef struct cais_pollfd {
    cais_socket_t *sock;    /* Target socket handle */
    int events;             /* Requested events: CAIS_SOCK_POLL_READ | WRITE */
    int revents;            /* Output events returned by kernel */
} cais_pollfd_t;

int cais_socket_poll(cais_pollfd_t *fds, size_t nfds, int timeout_ms);
```
* **Timeout Behavior:**
  * `timeout_ms < 0`: Blocks execution indefinitely until at least one event triggers.
  * `timeout_ms == 0`: Instant non-blocking poll.
  * `timeout_ms > 0`: Waits up to the specified millisecond deadline.
* **Returns:** Count of ready descriptors (> 0), `0` on timeout, or `CAIS_SOCK_ERROR` on failure.

---

### 3.6 Socket Mode & Timeout Configuration

```c
cais_socket_status_t cais_socket_set_mode(cais_socket_t *sock, cais_socket_mode_t mode);
cais_bool_t          cais_socket_is_blocking(const cais_socket_t *sock);
cais_bool_t          cais_socket_is_nonblocking(const cais_socket_t *sock);
cais_socket_status_t cais_socket_set_timeout(cais_socket_t *sock, int timeout_ms);
```

---

## 💻 4. Practical C89 Programming Examples

### Example 1: Non-Blocking TCP Echo Server with Multiplexing (`poll`)
Handles multiple concurrent clients without threads or processes:

```c
#include <cais/socket.h>
#include <stdio.h>
#include <string.h>

#define MAX_CLIENTS 32

int main(void) {
    char err[128];
    char buf[512];
    cais_socket_t *server;
    cais_socket_t *clients[MAX_CLIENTS];
    cais_pollfd_t pollfds[MAX_CLIENTS + 1];
    int client_count = 0;
    int i;

    memset(clients, 0, sizeof(clients));

    /* Bind non-blocking TCP server on port 8080 */
    server = cais_socket_bind("0.0.0.0", 8080, 16, CAIS_SOCK_NONBLOCKING, err, sizeof(err));
    if (!server) {
        fprintf(stderr, "Server bind failed: %s\n", err);
        return 1;
    }
    printf("Echo server listening on port 8080 (non-blocking)...\n");

    while (1) {
        int nfds = 0;
        int ready;

        /* Register listening server socket */
        pollfds[nfds].sock = server;
        pollfds[nfds].events = CAIS_SOCK_POLL_READ;
        pollfds[nfds].revents = 0;
        nfds++;

        /* Register active client sockets */
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i]) {
                pollfds[nfds].sock = clients[i];
                pollfds[nfds].events = CAIS_SOCK_POLL_READ;
                pollfds[nfds].revents = 0;
                nfds++;
            }
        }

        /* Wait up to 5000ms for network events */
        ready = cais_socket_poll(pollfds, nfds, 5000);
        if (ready < 0) break;
        if (ready == 0) continue;

        /* Check for new incoming connections */
        if (pollfds[0].revents & CAIS_SOCK_POLL_READ) {
            char client_ip[32];
            int client_port = 0;
            cais_socket_t *new_cli = cais_socket_accept(server, client_ip, sizeof(client_ip), 
                                                        &client_port, NULL, 0);
            if (new_cli) {
                cais_socket_set_mode(new_cli, CAIS_SOCK_NONBLOCKING);
                /* Add to client array */
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i]) {
                        clients[i] = new_cli;
                        client_count++;
                        printf("Connected: %s:%d (Slot %d)\n", client_ip, client_port, i);
                        break;
                    }
                }
            }
        }

        /* Process client read events */
        for (i = 1; i < nfds; i++) {
            if (pollfds[i].revents & CAIS_SOCK_POLL_READ) {
                size_t received = 0;
                size_t sent = 0;
                cais_socket_t *cli = pollfds[i].sock;
                cais_socket_status_t st = cais_socket_recv(cli, buf, sizeof(buf), &received);

                if (st == CAIS_SOCK_OK && received > 0) {
                    /* Echo back received bytes */
                    cais_socket_send(cli, buf, received, &sent);
                } else if (st == CAIS_SOCK_CLOSED || st == CAIS_SOCK_ERROR) {
                    /* Peer disconnected */
                    int j;
                    for (j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j] == cli) {
                            cais_socket_close(cli);
                            clients[j] = NULL;
                            client_count--;
                            printf("Disconnected: Slot %d\n", j);
                            break;
                        }
                    }
                }
            }
        }
    }

    cais_socket_close(server);
    return 0;
}
```

---

### Example 2: UDP Telemetry Receiver with Sender IP Tracking
Receives sensor metrics and acknowledges receipt back to the sender's origin port:

```c
#include <cais/socket.h>
#include <stdio.h>

int main(void) {
    char err[128];
    char buf[256];
    char sender_ip[32];
    int sender_port = 0;
    size_t received = 0;
    size_t sent = 0;
    cais_socket_t *udp = cais_socket_create(CAIS_SOCK_UDP, CAIS_SOCK_BLOCKING, err, sizeof(err));

    /* Send probe datagram */
    cais_socket_sendto(udp, "PING", 4, "127.0.0.1", 9000, &sent);

    /* Await response */
    if (cais_socket_recvfrom(udp, buf, sizeof(buf) - 1, sender_ip, sizeof(sender_ip), 
                             &sender_port, &received) == CAIS_SOCK_OK) {
        buf[received] = '\0';
        printf("Received %lu bytes from %s:%d: '%s'\n", 
               (unsigned long)received, sender_ip, sender_port, buf);
    }

    cais_socket_close(udp);
    return 0;
}
```

---

## 🌙 5. Practical Lua Programming Examples (Lua >= 5.4)

Loaded via `require("cais.socket")` or through the umbrella table `cais.socket`.

### 5.1 Constructor & Method Reference

| Function / Method     | Signature                                   | Description                                                        |
| :-------------------- | :------------------------------------------ | :----------------------------------------------------------------- |
| `sock.connect`        | `sock.connect(host, port [, mode])`         | Connects TCP client. Returns `client_obj` or `nil, err`.           |
| `sock.bind`           | `sock.bind(host, port [, backlog, mode])`   | Binds TCP server. Returns `server_obj` or `nil, err`.              |
| `sock.udp`            | `sock.udp([mode])`                          | Creates unbound UDP socket.                                        |
| `sock.udp_bind`       | `sock.udp_bind(host, port [, mode])`        | Creates bound UDP socket.                                          |
| `sock.select`         | `sock.select(readers, writers [, timeout])` | Multiplexes socket tables. Returns `ready_r, ready_w`.             |
| `sock:send` / `write` | `sock:send(data_str)`                       | Transmits stream data. Returns `sent_bytes` or `nil, err`.         |
| `sock:recv` / `read`  | `sock:recv([max_len])`                      | Receives incoming bytes. Returns `data_str` or `nil, err`.         |
| `sock:accept`         | `sock:accept()`                             | Accepts connection. Returns `client_sock, client_ip, client_port`. |
| `sock:sendto`         | `sock:sendto(data, host, port)`             | Emits UDP datagram to target. Returns `sent_bytes` or `nil, err`.  |
| `sock:recvfrom`       | `sock:recvfrom([max_len])`                  | Receives UDP datagram. Returns `data_str, src_ip, src_port`.       |
| `sock:set_mode`       | `sock:set_mode("blocking"                   | "nonblocking")`                                                    | Toggles blocking behavior. |
| `sock:set_timeout`    | `sock:set_timeout(timeout_ms)`              | Configures default I/O deadline.                                   |
| `sock:wait`           | `sock:wait("r"                              | "w"                                                                | "rw" [, timeout_ms])`      | Waits for readiness on single socket. |
| `sock:close`          | `sock:close()`                              | Shuts down socket and releases descriptor.                         |

---

### Example 1: Asynchronous Event-Driven TCP Chat Server Multiplexing 50+ Clients

```lua
local sock = require("cais.socket")

local server, err = sock.bind("127.0.0.1", 9999, 16, sock.mode.NONBLOCKING)
if not server then
    error("Failed to bind server: " .. tostring(err))
end

local clients = {}
print("Chat server active on 127.0.0.1:9999 (select multiplexing)...")

while true do
    -- Build reader array containing server + all active clients
    local readers = { server }
    for _, client in ipairs(clients) do
        table.insert(readers, client)
    end

    -- Multiplex events with 2000ms timeout
    local ready_readers = sock.select(readers, nil, 2000)

    for _, ready in ipairs(ready_readers) do
        if ready == server then
            -- New connection arrival
            local client, ip, port = server:accept()
            if client then
                client:set_mode("nonblocking")
                table.insert(clients, client)
                print(string.format("New client connected from %s:%d (Total: %d)", ip, port, #clients))
                client:send("Welcome to cais event-driven chat!\n")
            end
        else
            -- Data from existing client
            local msg, recv_err = ready:recv(512)
            if msg then
                print("Broadcasting message: " .. msg:gsub("[\r\n]+", ""))
                -- Broadcast to all other connected peers
                for _, other in ipairs(clients) do
                    if other ~= ready then
                        other:send(msg)
                    end
                end
            else
                -- Disconnection or error
                ready:close()
                for idx, c in ipairs(clients) do
                    if c == ready then
                        table.remove(clients, idx)
                        print("Client disconnected. Remaining: " .. #clients)
                        break
                    end
                end
            end
        end
    end
end
```

---

### Example 2: UDP Telemetry Heartbeat Monitor

```lua
local sock = require("cais.socket")

-- Bind UDP listener on port 8888
local listener, err = sock.udp_bind("0.0.0.0", 8888)
if not listener then
    error("Failed to bind UDP listener: " .. tostring(err))
end

print("Listening for UDP sensor heartbeats on port 8888...")

while true do
    -- Await datagram with 5000ms timeout using wait()
    local ready = listener:wait("r", 5000)
    if ready then
        local packet, sender_ip, sender_port = listener:recvfrom(1024)
        if packet then
            print(string.format("[%s:%d] Heartbeat: %s", sender_ip, sender_port, packet))
            -- Send ACK back to sender
            listener:sendto("ACK:OK\n", sender_ip, sender_port)
        end
    else
        print("Heartbeat timeout: No sensor packets received in the last 5 seconds.")
    end
end
```