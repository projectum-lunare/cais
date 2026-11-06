/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_TLS_H
#define CAIS_TLS_H

#include <stddef.h>
#include <cais/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct cais_socket;

/*
 * =============================================================================
 * Enumerations & Status Codes
 * =============================================================================
 */

/* TLS operation status codes */
typedef enum cais_tls_status {
    CAIS_TLS_OK         = 0,  /* Operation completed successfully */
    CAIS_TLS_ERROR      = -1, /* Fatal or cryptographic/protocol error */
    CAIS_TLS_WANT_READ  = 1,  /* Blocked: underlying transport needs to read data */
    CAIS_TLS_WANT_WRITE = 2,  /* Blocked: underlying transport needs to write data */
    CAIS_TLS_CLOSED     = 3   /* Connection closed by peer (orderly TLS CloseNotify) */
} cais_tls_status_t;

/* TLS protocol role */
typedef enum cais_tls_role {
    CAIS_TLS_CLIENT = 0, /* Local party connects as client */
    CAIS_TLS_SERVER = 1  /* Local party listens/accepts as server */
} cais_tls_role_t;

/* TLS certificate verification policy */
typedef enum cais_tls_verify {
    CAIS_TLS_VERIFY_NONE     = 0, /* Insecure: skip certificate validation (ideal for vintage/test) */
    CAIS_TLS_VERIFY_REQUIRED = 1  /* Strict: validate chain against trust anchors */
} cais_tls_verify_t;

/* TLS backend engine identifier */
typedef enum cais_tls_backend_type {
    CAIS_TLS_BACKEND_NONE     = 0, /* Subsystem disabled or stub */
    CAIS_TLS_BACKEND_BEARSSL  = 1, /* BearSSL (strict C89, ultra-lightweight) */
    CAIS_TLS_BACKEND_WOLFSSL  = 2, /* wolfSSL (embedded TLS 1.3) */
    CAIS_TLS_BACKEND_MBEDTLS  = 3, /* mbedTLS (ARM ecosystem) */
    CAIS_TLS_BACKEND_SCHANNEL = 4  /* Windows native SChannel */
} cais_tls_backend_type_t;

/*
 * =============================================================================
 * Configuration & Context Structures
 * =============================================================================
 */

/* Trust Anchor / Root CA configuration */
typedef struct cais_tls_ca {
    const void *data;  /* DER or PEM encoded root certificate(s) buffer */
    size_t      len;   /* Length of certificate data in bytes */
    const char *path;  /* Optional filesystem path to CA bundle file */
} cais_tls_ca_t;

/* Local certificate keypair (server mode or client mTLS) */
typedef struct cais_tls_keypair {
    const void *cert_data; /* Certificate chain (PEM or DER buffer) */
    size_t      cert_len;
    const void *key_data;  /* Private key (PEM or DER buffer) */
    size_t      key_len;
} cais_tls_keypair_t;

/* High-level TLS configuration options */
typedef struct cais_tls_config {
    cais_tls_role_t     role;        /* Client or Server */
    cais_tls_verify_t   verify;      /* Verification mode */
    const char         *server_name; /* Server Name Indication (SNI) / Hostname verification */
    cais_tls_ca_t       ca;          /* Root CA trust store */
    cais_tls_keypair_t  keypair;     /* Local credentials */
    const char        **alpn;        /* NULL-terminated array of ALPN strings (optional) */
} cais_tls_config_t;

/* Custom I/O callbacks for non-socket transports (serial, memory, pipes) */
typedef int (*cais_tls_read_cb)(void *user_data, void *buf, size_t len, size_t *read_bytes);
typedef int (*cais_tls_write_cb)(void *user_data, const void *buf, size_t len, size_t *written_bytes);

/* Opaque TLS session context */
typedef struct cais_tls {
    void               *backend_ctx;   /* Engine-specific private handle */
    cais_tls_config_t   config;        /* Active configuration copy */
    struct cais_socket *sock;          /* Attached cais_socket_t handle (if used) */
    cais_tls_read_cb    read_cb;       /* Custom read callback */
    cais_tls_write_cb   write_cb;      /* Custom write callback */
    void               *io_user_data;  /* Pointer forwarded to custom I/O callbacks */
    int                 handshake_done;/* 1 if handshake was successfully completed */
    int                 closed;        /* 1 if session has been closed */
} cais_tls_t;

/*
 * =============================================================================
 * Introspection & Metadata API
 * =============================================================================
 */

/* Returns the active TLS backend engine type */
cais_tls_backend_type_t cais_tls_backend(void);

/* Returns the human-readable name of the active TLS backend */
const char *cais_tls_backend_name(void);

/* Returns 1 if TLS subsystem is compiled and operational, 0 otherwise */
int cais_tls_is_supported(void);

/*
 * =============================================================================
 * Lifecycle & Context Management
 * =============================================================================
 */

/*
 * Populates a config structure with safe defaults (Client mode, VERIFY_REQUIRED).
 */
void cais_tls_config_init(cais_tls_config_t *conf, cais_tls_role_t role);

/*
 * Initializes a TLS context using the given configuration.
 * Returns CAIS_TLS_OK on success, or CAIS_TLS_ERROR on failure.
 */
int cais_tls_init(cais_tls_t *tls, const cais_tls_config_t *conf, char *err_buf, size_t err_cap);

/*
 * Binds a cais_socket_t handle to the TLS context for transport I/O.
 */
int cais_tls_set_socket(cais_tls_t *tls, struct cais_socket *sock);

/*
 * Binds custom read/write callbacks for arbitrary transports (serial, memory, etc.).
 */
int cais_tls_set_io(cais_tls_t *tls, cais_tls_read_cb read_cb, cais_tls_write_cb write_cb, void *user_data);

/*
 * Executes or continues the TLS handshake.
 *
 * For blocking transports, performs complete handshake and returns CAIS_TLS_OK or CAIS_TLS_ERROR.
 * For non-blocking transports, may return CAIS_TLS_WANT_READ or CAIS_TLS_WANT_WRITE.
 */
int cais_tls_handshake(cais_tls_t *tls, char *err_buf, size_t err_cap);

/*
 * Sends plaintext data through the encrypted TLS channel.
 *
 * `written_bytes` receives the number of plaintext bytes accepted and encrypted.
 * Returns CAIS_TLS_OK, CAIS_TLS_WANT_WRITE, CAIS_TLS_WANT_READ, or CAIS_TLS_ERROR.
 */
int cais_tls_send(cais_tls_t *tls, const void *data, size_t len, size_t *written_bytes, char *err_buf, size_t err_cap);

/*
 * Receives decrypted plaintext data from the TLS channel.
 *
 * `read_bytes` receives the number of plaintext bytes read.
 * Returns CAIS_TLS_OK, CAIS_TLS_WANT_READ, CAIS_TLS_WANT_WRITE, CAIS_TLS_CLOSED, or CAIS_TLS_ERROR.
 */
int cais_tls_recv(cais_tls_t *tls, void *buffer, size_t len, size_t *read_bytes, char *err_buf, size_t err_cap);

/*
 * Sends a TLS CloseNotify alert and initiates orderly session shutdown.
 */
int cais_tls_close(cais_tls_t *tls);

/*
 * Releases all backend memory and resets the TLS context.
 */
void cais_tls_free(cais_tls_t *tls);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_TLS_H */
