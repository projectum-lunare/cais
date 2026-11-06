# Modular TLS Secure Transport (`cais.tls`)

The `tls` subsystem provides an extensible, modular, and standard-compliant **Transport Layer Security (TLS)** substrate. It is designed to secure stream transports across modern POSIX systems, Windows, and vintage embedded platforms using pluggable, lightweight cryptographic backends. Supported protocol versions (such as TLS 1.0, 1.1, 1.2, or 1.3), cipher suites, and cryptographic capabilities are defined by the active backend driver rather than the abstraction layer itself.

While `cais.noise` remains our primary recommendation for certificate-less, zero-overhead peer-to-peer secure communications, `cais.tls` enables mandatory interoperability with the broader Internet ecosystem (HTTPS, MQTTS, secure WebSockets, FTPS) where standard X.509 certificate validation and legacy TLS protocol conformance are required.

---

## ⚡ 1. Architectural Principles & Guarantees

### 1.1 Pluggable Backend Architecture (VTable Driver Model)
The subsystem is cleanly separated into two layers:
1. **Generic Dispatch API (`include/cais/tls.h` / `src/tls/tls.c`):** Exposes a stable, high-level C89 API to consumers and Lua bindings.
2. **Backend Driver Interface (`src/tls/tls_internal.h`):** Internal function pointer vtable (`cais_tls_backend_ops_t`) implemented by individual crypto engine drivers.

```
┌────────────────────────────────────────────────────────┐
│             cais Application / Lua Binding             │
└───────────────────────────┬────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│      Generic Dispatch Layer (cais_tls_* API)           │
└───────────────────────────┬────────────────────────────┘
                            │ VTable ops
              ┌─────────────┼─────────────┐
              ▼             ▼             ▼
       ┌─────────────┐┌───────────┐┌─────────────┐
       │   BearSSL   ││  wolfSSL  ││  mbedTLS    │
       │   (Driver)  ││ (Future)  ││  (Future)   │
       └─────────────┘└───────────┘└─────────────┘
```

This ensures zero vendor lock-in: applications and scripts interact with identical function calls and semantics regardless of whether the underlying engine is **BearSSL**, **wolfSSL**, **mbedTLS**, or a stub on unsupported platforms.

### 1.2 Transport-Agnostic I/O Decoupling
`cais.tls` does not mandate raw POSIX or Winsock socket handles. Instead, it supports two decoupled I/O models:

* **Direct Socket Wrapping:** Pass an existing `cais_socket_t*` via `cais_tls_wrap_socket()`. The engine handles non-blocking socket reads and writes automatically.
* **Custom Stream Callbacks:** Bind arbitrary read and write callbacks via `cais_tls_set_io()`. This allows TLS to run over:
  * Physical serial UART lines (`cais_serial_t`).
  * In-memory byte buffers / ring buffers.
  * Inter-process UNIX domain pipes or IPC channels.
  * Custom packetized mesh radio protocols.

---

## 🔌 2. Available Drivers

Protocol version support, cipher suites, and runtime capabilities depend directly on the chosen backend driver:

| Backend Driver | Target TLS Versions | X.509 Support | Memory Footprint | Primary Target Environments |
| :------------- | :------------------ | :------------ | :--------------- | :-------------------------- |
| **`bearssl`**  | TLS 1.0, 1.1, 1.2   | X.509 & None  | ~25 KB RAM       | Embedded, Vintage x86/ARM/m68k, POSIX, Windows |
| **`stub`**     | None (Fallback)     | None          | 0 KB             | Platforms without TLS requirements or `--tls=n` |

*(Additional drivers, such as `wolfssl` or `mbedtls`, may be plugged in to provide TLS 1.3).*

### BearSSL Driver Highlights
* **Zero Dynamic Heap Allocations During I/O:** BearSSL operates strictly on static or caller-allocated memory buffers (`br_ssl_client_context` and internal I/O buffers).
* **Strict ANSI C89 Compatibility:** All header inlines and helper structs compile without warnings under `-std=c89` / `-pedantic`.
* **Insecure / Embedded Verification Mode:** Supports `CAIS_TLS_VERIFY_NONE` via a custom non-validating X.509 minimal decoder, allowing lightweight testing, embedded telemetry, and systems without access to a full 300 KB Root CA bundle.

---

## 🔄 3. Non-Blocking Event Semantics

All handshake and I/O functions return explicit status codes designed to integrate cleanly into non-blocking event loops (`poll`, `select`, or `cais_socket_poll`):

* `CAIS_TLS_OK`: The requested operation completed successfully (bytes transferred or handshake fully established).
* `CAIS_TLS_WANT_READ`: The underlying transport needs to receive more records from the peer before TLS state can advance. Register socket/I/O for reading.
* `CAIS_TLS_WANT_WRITE`: The TLS engine has buffered outbound handshake or application data that must be flushed to the peer. Register socket/I/O for writing.
* `CAIS_TLS_CLOSED`: The TLS session received a graceful `close_notify` alert from the remote peer.
* `CAIS_TLS_ERROR`: A protocol violation, certificate verification failure, or underlying I/O error occurred.

---

## 💻 4. C API Usage Guide

### 4.1 Connecting to an HTTPS Server over a `cais_socket_t`

```c
#include <stdio.h>
#include <cais/cais.h>
#include <cais/tls.h>

int main(void) {
    char err_buf[128];
    cais_socket_t *sock;
    cais_tls_t *tls;
    cais_tls_config_t cfg;
    cais_tls_status_t status;
    char buffer[1024];
    size_t transferred = 0;

    /* 1. Establish raw TCP connection */
    sock = cais_socket_connect("1.1.1.1", 443, CAIS_SOCK_BLOCKING, err_buf, sizeof(err_buf));
    if (!sock) {
        fprintf(stderr, "Socket error: %s\n", err_buf);
        return 1;
    }

    /* 2. Configure TLS client */
    cais_tls_config_init(&cfg);
    cfg.role = CAIS_TLS_CLIENT;
    cfg.verify = CAIS_TLS_VERIFY_NONE; /* Insecure mode for testing */
    cfg.server_name = "cloudflare-dns.com";

    /* 3. Wrap socket */
    tls = cais_tls_wrap_socket(sock, &cfg, err_buf, sizeof(err_buf));
    if (!tls) {
        fprintf(stderr, "TLS wrap error: %s\n", err_buf);
        cais_socket_close(sock);
        return 1;
    }

    /* 4. Complete TLS handshake */
    do {
        status = cais_tls_handshake(tls);
    } while (status == CAIS_TLS_WANT_READ || status == CAIS_TLS_WANT_WRITE);

    if (status != CAIS_TLS_OK) {
        fprintf(stderr, "TLS handshake failed: %s\n", cais_tls_get_error(tls));
        cais_tls_destroy(tls);
        cais_socket_close(sock);
        return 1;
    }

    /* 5. Transmit HTTP request */
    cais_tls_send(tls, "GET /dns-query?name=example.com HTTP/1.1\r\nHost: cloudflare-dns.com\r\n\r\n", 68, &transferred);

    /* 6. Receive response */
    status = cais_tls_recv(tls, buffer, sizeof(buffer) - 1, &transferred);
    if (status == CAIS_TLS_OK && transferred > 0) {
        buffer[transferred] = '\0';
        printf("Received HTTP response:\n%s\n", buffer);
    }

    /* 7. Teardown */
    cais_tls_close(tls);
    cais_tls_destroy(tls);
    cais_socket_close(sock);
    return 0;
}
```

---

## ⚙️ 5. Build Configuration Options

To configure TLS support in `xmake`:

```bash
# Enable default BearSSL backend:
xmake f --tls=y --tls_backend=bearssl
xmake

# Build without TLS (pure lightweight footprint):
xmake f --tls=n
xmake
```
