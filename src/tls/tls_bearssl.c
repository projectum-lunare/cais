/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "tls_internal.h"

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#if defined(__GNUC__) || defined(__clang__)
#ifndef inline
#define inline __inline__
#endif
#elif defined(_MSC_VER)
#ifndef inline
#define inline __inline
#endif
#else
#ifndef inline
#define inline
#endif
#endif
#endif

#include <bearssl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

/*
 * Insecure X.509 Validator
 * Extracts public key from the first server certificate without CA verification.
 */
typedef struct cais_insecure_x509_ctx {
    const br_x509_class *vtable;
    br_x509_decoder_context decoder;
    int first_cert;
    br_x509_pkey pkey;
} cais_insecure_x509_ctx_t;

static void insecure_start_chain(const br_x509_class **ctx, const char *server_name)
{
    cais_insecure_x509_ctx_t *x = (cais_insecure_x509_ctx_t *)ctx;
    (void)server_name;
    x->first_cert = 1;
}

static void insecure_start_cert(const br_x509_class **ctx, uint32_t length)
{
    cais_insecure_x509_ctx_t *x = (cais_insecure_x509_ctx_t *)ctx;
    (void)length;
    if (x->first_cert) {
        br_x509_decoder_init(&x->decoder, 0, 0);
    }
}

static void insecure_append(const br_x509_class **ctx, const unsigned char *buf, size_t len)
{
    cais_insecure_x509_ctx_t *x = (cais_insecure_x509_ctx_t *)ctx;
    if (x->first_cert) {
        br_x509_decoder_push(&x->decoder, buf, len);
    }
}

static void insecure_end_cert(const br_x509_class **ctx)
{
    cais_insecure_x509_ctx_t *x = (cais_insecure_x509_ctx_t *)ctx;
    if (x->first_cert) {
        const br_x509_pkey *pk = br_x509_decoder_get_pkey(&x->decoder);
        if (pk) {
            x->pkey = *pk;
        }
        x->first_cert = 0;
    }
}

static unsigned insecure_end_chain(const br_x509_class **ctx)
{
    (void)ctx;
    return 0; /* 0 indicates success in BearSSL X.509 */
}

static const br_x509_pkey *insecure_get_pkey(const br_x509_class *const *ctx, unsigned *usages)
{
    cais_insecure_x509_ctx_t *x = (cais_insecure_x509_ctx_t *)ctx;
    if (usages) {
        *usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN;
    }
    return &x->pkey;
}

static const br_x509_class s_insecure_x509_vtable = {
    sizeof(cais_insecure_x509_ctx_t),
    insecure_start_chain,
    insecure_start_cert,
    insecure_append,
    insecure_end_cert,
    insecure_end_chain,
    insecure_get_pkey
};

/*
 * BearSSL Backend Context
 */
typedef struct cais_bearssl_ctx {
    br_ssl_client_context client_ctx;
    br_x509_minimal_context x509_min;
    cais_insecure_x509_ctx_t x509_insecure;
    unsigned char iobuf[BR_SSL_BUFSIZE_BIDI];
} cais_bearssl_ctx_t;

/*
 * Low-level I/O pump between BearSSL state machine and cais transport callbacks
 */
static int bearssl_pump(cais_tls_t *tls, char *err_buf, size_t err_cap)
{
    cais_bearssl_ctx_t *bctx = (cais_bearssl_ctx_t *)tls->backend_ctx;
    br_ssl_engine_context *eng = &bctx->client_ctx.eng;

    for (;;) {
        unsigned state = br_ssl_engine_current_state(eng);

        if (state & BR_SSL_CLOSED) {
            int err = br_ssl_engine_last_error(eng);
            if (err == BR_ERR_OK) {
                return CAIS_TLS_CLOSED;
            }
            if (err_buf && err_cap > 0) {
                sprintf(err_buf, "BearSSL engine closed with error %d", err);
            }
            return CAIS_TLS_ERROR;
        }

        if (state & BR_SSL_SENDREC) {
            size_t len = 0;
            unsigned char *buf = br_ssl_engine_sendrec_buf(eng, &len);
            size_t sent = 0;
            int res;

            if (!tls->write_cb) return CAIS_TLS_ERROR;
            res = tls->write_cb(tls->io_user_data, buf, len, &sent);
            if (res == CAIS_TLS_OK && sent > 0) {
                br_ssl_engine_sendrec_ack(eng, sent);
                continue;
            }
            if (res == CAIS_TLS_WANT_WRITE || res == CAIS_TLS_WANT_READ) {
                return res;
            }
            if (res == CAIS_TLS_CLOSED) {
                br_ssl_engine_close(eng);
                return CAIS_TLS_CLOSED;
            }
            if (err_buf && err_cap > 0) {
                strncpy(err_buf, "Transport write failed during TLS handshake/pump", err_cap - 1);
                err_buf[err_cap - 1] = '\0';
            }
            return CAIS_TLS_ERROR;
        }

        if (state & BR_SSL_RECVREC) {
            size_t len = 0;
            unsigned char *buf = br_ssl_engine_recvrec_buf(eng, &len);
            size_t read_bytes = 0;
            int res;

            if (!tls->read_cb) return CAIS_TLS_ERROR;
            res = tls->read_cb(tls->io_user_data, buf, len, &read_bytes);
            if (res == CAIS_TLS_OK && read_bytes > 0) {
                br_ssl_engine_recvrec_ack(eng, read_bytes);
                continue;
            }
            if (res == CAIS_TLS_WANT_READ || res == CAIS_TLS_WANT_WRITE) {
                return res;
            }
            if (res == CAIS_TLS_CLOSED) {
                br_ssl_engine_close(eng);
                continue;
            }
            if (err_buf && err_cap > 0) {
                strncpy(err_buf, "Transport read failed during TLS handshake/pump", err_cap - 1);
                err_buf[err_cap - 1] = '\0';
            }
            return CAIS_TLS_ERROR;
        }

        break;
    }

    return CAIS_TLS_OK;
}

static int bearssl_init(cais_tls_t *tls, const cais_tls_config_t *conf, char *err_buf, size_t err_cap)
{
    cais_bearssl_ctx_t *bctx;

    bctx = (cais_bearssl_ctx_t *)calloc(1, sizeof(cais_bearssl_ctx_t));
    if (!bctx) {
        if (err_buf && err_cap > 0) {
            strncpy(err_buf, "Memory allocation failed for BearSSL context", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
        return CAIS_TLS_ERROR;
    }

    if (conf->role == CAIS_TLS_CLIENT) {
        br_x509_minimal_init_full(&bctx->x509_min, NULL, 0);
        br_ssl_client_init_full(&bctx->client_ctx, &bctx->x509_min, NULL, 0);

        if (conf->verify == CAIS_TLS_VERIFY_NONE) {
            bctx->x509_insecure.vtable = &s_insecure_x509_vtable;
            br_ssl_engine_set_x509(&bctx->client_ctx.eng, (const br_x509_class **)&bctx->x509_insecure);
        }

        br_ssl_engine_set_buffer(&bctx->client_ctx.eng, bctx->iobuf, sizeof(bctx->iobuf), 1);
        br_ssl_client_reset(&bctx->client_ctx, conf->server_name ? conf->server_name : "", 0);
    } else {
        /* Server role */
        free(bctx);
        if (err_buf && err_cap > 0) {
            strncpy(err_buf, "BearSSL server mode requires certificate and private key setup", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
        return CAIS_TLS_ERROR;
    }

    tls->backend_ctx = bctx;
    return CAIS_TLS_OK;
}

static int bearssl_handshake(cais_tls_t *tls, char *err_buf, size_t err_cap)
{
    cais_bearssl_ctx_t *bctx = (cais_bearssl_ctx_t *)tls->backend_ctx;
    br_ssl_engine_context *eng;

    if (!bctx) return CAIS_TLS_ERROR;
    eng = &bctx->client_ctx.eng;

    for (;;) {
        unsigned state = br_ssl_engine_current_state(eng);

        if (state & (BR_SSL_SENDAPP | BR_SSL_RECVAPP)) {
            tls->handshake_done = 1;
            return CAIS_TLS_OK;
        }

        if (state & BR_SSL_CLOSED) {
            int err = br_ssl_engine_last_error(eng);
            if (err_buf && err_cap > 0) {
                sprintf(err_buf, "TLS handshake failed (BearSSL error %d)", err);
            }
            return CAIS_TLS_ERROR;
        }

        if (state & (BR_SSL_SENDREC | BR_SSL_RECVREC)) {
            int res = bearssl_pump(tls, err_buf, err_cap);
            if (res != CAIS_TLS_OK) {
                return res;
            }
        } else {
            break;
        }
    }

    if (br_ssl_engine_current_state(eng) & (BR_SSL_SENDAPP | BR_SSL_RECVAPP)) {
        tls->handshake_done = 1;
        return CAIS_TLS_OK;
    }

    return CAIS_TLS_WANT_READ;
}

static int bearssl_send(cais_tls_t *tls, const void *data, size_t len, size_t *written_bytes, char *err_buf, size_t err_cap)
{
    cais_bearssl_ctx_t *bctx = (cais_bearssl_ctx_t *)tls->backend_ctx;
    br_ssl_engine_context *eng;
    unsigned state;
    size_t to_write, cap;
    unsigned char *buf;
    int pump_res;

    if (!bctx) return CAIS_TLS_ERROR;
    eng = &bctx->client_ctx.eng;

    pump_res = bearssl_pump(tls, err_buf, err_cap);
    if (pump_res != CAIS_TLS_OK && pump_res != CAIS_TLS_WANT_READ && pump_res != CAIS_TLS_WANT_WRITE) {
        return pump_res;
    }

    state = br_ssl_engine_current_state(eng);
    if (!(state & BR_SSL_SENDAPP)) {
        if (state & BR_SSL_CLOSED) return CAIS_TLS_CLOSED;
        return CAIS_TLS_WANT_WRITE;
    }

    buf = br_ssl_engine_sendapp_buf(eng, &cap);
    to_write = (len < cap) ? len : cap;
    if (to_write > 0) {
        memcpy(buf, data, to_write);
        br_ssl_engine_sendapp_ack(eng, to_write);
        br_ssl_engine_flush(eng, 0);
        if (written_bytes) *written_bytes = to_write;

        /* Attempt immediate flush to network */
        bearssl_pump(tls, NULL, 0);
        return CAIS_TLS_OK;
    }

    return CAIS_TLS_WANT_WRITE;
}

static int bearssl_recv(cais_tls_t *tls, void *buffer, size_t len, size_t *read_bytes, char *err_buf, size_t err_cap)
{
    cais_bearssl_ctx_t *bctx = (cais_bearssl_ctx_t *)tls->backend_ctx;
    br_ssl_engine_context *eng;
    unsigned state;
    size_t avail, to_read;
    unsigned char *buf;
    int pump_res;

    if (!bctx) return CAIS_TLS_ERROR;
    eng = &bctx->client_ctx.eng;

    state = br_ssl_engine_current_state(eng);
    if (!(state & BR_SSL_RECVAPP)) {
        pump_res = bearssl_pump(tls, err_buf, err_cap);
        if (pump_res != CAIS_TLS_OK && pump_res != CAIS_TLS_WANT_READ && pump_res != CAIS_TLS_WANT_WRITE) {
            return pump_res;
        }
        state = br_ssl_engine_current_state(eng);
    }

    if (state & BR_SSL_RECVAPP) {
        buf = br_ssl_engine_recvapp_buf(eng, &avail);
        to_read = (len < avail) ? len : avail;
        memcpy(buffer, buf, to_read);
        br_ssl_engine_recvapp_ack(eng, to_read);
        if (read_bytes) *read_bytes = to_read;
        return CAIS_TLS_OK;
    }

    if (state & BR_SSL_CLOSED) {
        return CAIS_TLS_CLOSED;
    }

    return CAIS_TLS_WANT_READ;
}

static int bearssl_close(cais_tls_t *tls)
{
    cais_bearssl_ctx_t *bctx = (cais_bearssl_ctx_t *)tls->backend_ctx;
    if (bctx) {
        br_ssl_engine_close(&bctx->client_ctx.eng);
        bearssl_pump(tls, NULL, 0);
    }
    return CAIS_TLS_OK;
}

static void bearssl_free(cais_tls_t *tls)
{
    if (tls && tls->backend_ctx) {
        free(tls->backend_ctx);
        tls->backend_ctx = NULL;
    }
}

static const cais_tls_backend_ops_t s_bearssl_ops = {
    CAIS_TLS_BACKEND_BEARSSL,
    "bearssl",
    bearssl_init,
    bearssl_handshake,
    bearssl_send,
    bearssl_recv,
    bearssl_close,
    bearssl_free
};

const cais_tls_backend_ops_t *cais_tls_get_backend_ops(void)
{
    return &s_bearssl_ops;
}
