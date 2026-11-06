/*
 * cais - Communication Abstraction & I/O Substrate
 * Native C API Unit Tests: TLS Secure Transport Subsystem
 */

#include "greatest.h"
#include <cais/cais.h>
#include <string.h>

#ifndef CAIS_NO_TLS

/* 1. Introspection & Configuration Tests */
TEST test_tls_introspection(void)
{
    cais_tls_backend_type_t btype;
    const char *bname;

    ASSERT_EQ(1, cais_tls_is_supported());

    btype = cais_tls_backend();
    ASSERT_EQ(CAIS_TLS_BACKEND_BEARSSL, btype);

    bname = cais_tls_backend_name();
    ASSERT(bname != NULL);
    ASSERT_STR_EQ("bearssl", bname);

    PASS();
}

TEST test_tls_config_defaults(void)
{
    cais_tls_config_t conf;

    cais_tls_config_init(&conf, CAIS_TLS_CLIENT);
    ASSERT_EQ(CAIS_TLS_CLIENT, conf.role);
    ASSERT_EQ(CAIS_TLS_VERIFY_REQUIRED, conf.verify);
    ASSERT_EQ(NULL, conf.server_name);

    cais_tls_config_init(&conf, CAIS_TLS_SERVER);
    ASSERT_EQ(CAIS_TLS_SERVER, conf.role);
    ASSERT_EQ(CAIS_TLS_VERIFY_NONE, conf.verify);

    PASS();
}

TEST test_tls_init_and_lifecycle(void)
{
    cais_tls_config_t conf;
    cais_tls_t tls;
    char err[128];
    int res;

    cais_tls_config_init(&conf, CAIS_TLS_CLIENT);
    conf.verify = CAIS_TLS_VERIFY_NONE;
    conf.server_name = "example.com";

    res = cais_tls_init(&tls, &conf, err, sizeof(err));
    ASSERT_EQ(CAIS_TLS_OK, res);
    ASSERT_EQ(0, tls.handshake_done);

    /* Operations before handshake must fail gracefully */
    res = cais_tls_send(&tls, "hello", 5, NULL, err, sizeof(err));
    ASSERT_EQ(CAIS_TLS_ERROR, res);

    res = cais_tls_recv(&tls, err, sizeof(err), NULL, err, sizeof(err));
    ASSERT_EQ(CAIS_TLS_ERROR, res);

    /* Close and free */
    ASSERT_EQ(CAIS_TLS_OK, cais_tls_close(&tls));
    cais_tls_free(&tls);

    PASS();
}

/* Mock memory transport for deterministic non-blocking handshake test */
typedef struct mock_pipe {
    unsigned char buf[4096];
    size_t len;
    size_t pos;
} mock_pipe_t;

typedef struct mock_pipe_pair {
    mock_pipe_t rx;
    mock_pipe_t tx;
} mock_pipe_pair_t;

static int mock_read_cb(void *user_data, void *buf, size_t len, size_t *read_bytes)
{
    mock_pipe_pair_t *pair = (mock_pipe_pair_t *)user_data;
    size_t avail = pair->rx.len - pair->rx.pos;
    size_t to_copy;

    if (avail == 0) {
        if (read_bytes) *read_bytes = 0;
        return CAIS_TLS_WANT_READ;
    }

    to_copy = (len < avail) ? len : avail;
    memcpy(buf, pair->rx.buf + pair->rx.pos, to_copy);
    pair->rx.pos += to_copy;
    if (read_bytes) *read_bytes = to_copy;
    return CAIS_TLS_OK;
}

static int mock_write_cb(void *user_data, const void *buf, size_t len, size_t *written_bytes)
{
    mock_pipe_pair_t *pair = (mock_pipe_pair_t *)user_data;
    size_t cap = sizeof(pair->tx.buf) - pair->tx.len;
    size_t to_copy = (len < cap) ? len : cap;

    if (to_copy == 0) {
        if (written_bytes) *written_bytes = 0;
        return CAIS_TLS_WANT_WRITE;
    }

    memcpy(pair->tx.buf + pair->tx.len, buf, to_copy);
    pair->tx.len += to_copy;
    if (written_bytes) *written_bytes = to_copy;
    return CAIS_TLS_OK;
}

TEST test_tls_custom_io_handshake_start(void)
{
    cais_tls_config_t conf;
    cais_tls_t tls;
    mock_pipe_pair_t pipes;
    char err[128];
    int res;

    memset(&pipes, 0, sizeof(pipes));

    cais_tls_config_init(&conf, CAIS_TLS_CLIENT);
    conf.verify = CAIS_TLS_VERIFY_NONE;
    conf.server_name = "test.local";

    res = cais_tls_init(&tls, &conf, err, sizeof(err));
    ASSERT_EQ(CAIS_TLS_OK, res);

    /* Bind custom I/O */
    res = cais_tls_set_io(&tls, mock_read_cb, mock_write_cb, &pipes);
    ASSERT_EQ(CAIS_TLS_OK, res);

    /* Starting handshake will produce TLS ClientHello into pipes.tx, then return WANT_READ */
    res = cais_tls_handshake(&tls, err, sizeof(err));
    ASSERT_EQ(CAIS_TLS_WANT_READ, res);

    /* Verify ClientHello was emitted by BearSSL */
    ASSERT(pipes.tx.len > 0);
    /* TLS record header: byte 0 = 0x16 (Handshake), byte 1 = 0x03 (TLS major) */
    ASSERT_EQ(0x16, pipes.tx.buf[0]);
    ASSERT_EQ(0x03, pipes.tx.buf[1]);

    cais_tls_close(&tls);
    cais_tls_free(&tls);

    PASS();
}

SUITE(tls_suite)
{
    RUN_TEST(test_tls_introspection);
    RUN_TEST(test_tls_config_defaults);
    RUN_TEST(test_tls_init_and_lifecycle);
    RUN_TEST(test_tls_custom_io_handshake_start);
}

#endif /* CAIS_NO_TLS */
