/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 * Mozilla Public License Version 2.0
 */

#include "test_common.h"
#include <parson.h>

TEST test_noise_keygen(void)
{
    unsigned char priv1[CAIS_NOISE_KEY_LEN];
    unsigned char pub1[CAIS_NOISE_KEY_LEN];
    unsigned char priv2[CAIS_NOISE_KEY_LEN];
    unsigned char pub2[CAIS_NOISE_KEY_LEN];
    char err[128];

    ASSERT_EQ(CAIS_OK, cais_noise_keygen(priv1, pub1, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_keygen(priv2, pub2, err, sizeof(err)));

    /* Keys must be distinct */
    ASSERT(memcmp(priv1, priv2, CAIS_NOISE_KEY_LEN) != 0);
    ASSERT(memcmp(pub1, pub2, CAIS_NOISE_KEY_LEN) != 0);

    PASS();
}

TEST test_noise_nn_handshake(void)
{
    cais_noise_t *client;
    cais_noise_t *server;
    char err[128];
    unsigned char msg1[128];
    unsigned char msg2[128];
    unsigned char payload_out[128];
    size_t msg1_len = 0;
    size_t msg2_len = 0;
    size_t plen = 0;

    /* Transport buffers */
    unsigned char cipher[128];
    unsigned char plain[128];
    size_t clen = 0;

    client = cais_noise_create(CAIS_NOISE_NN, CAIS_NOISE_INITIATOR, err, sizeof(err));
    server = cais_noise_create(CAIS_NOISE_NN, CAIS_NOISE_RESPONDER, err, sizeof(err));
    ASSERT(client != NULL);
    ASSERT(server != NULL);

    ASSERT_EQ(CAIS_OK, cais_noise_start(client, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_start(server, err, sizeof(err)));

    /* Step 1: Client writes -> Server reads */
    ASSERT_EQ(CAIS_NOISE_ACTION_WRITE_MSG, cais_noise_get_action(client));
    ASSERT_EQ(CAIS_NOISE_ACTION_READ_MSG, cais_noise_get_action(server));

    ASSERT_EQ(CAIS_OK, cais_noise_write_message(client, "ping1", 5, msg1, sizeof(msg1), &msg1_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(server, msg1, msg1_len, payload_out, sizeof(payload_out), &plen, err, sizeof(err)));
    ASSERT_EQ(5, plen);
    ASSERT(memcmp(payload_out, "ping1", 5) == 0);

    /* Step 2: Server writes -> Client reads */
    ASSERT_EQ(CAIS_NOISE_ACTION_WRITE_MSG, cais_noise_get_action(server));
    ASSERT_EQ(CAIS_NOISE_ACTION_READ_MSG, cais_noise_get_action(client));

    ASSERT_EQ(CAIS_OK, cais_noise_write_message(server, "pong2", 5, msg2, sizeof(msg2), &msg2_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(client, msg2, msg2_len, payload_out, sizeof(payload_out), &plen, err, sizeof(err)));
    ASSERT_EQ(5, plen);
    ASSERT(memcmp(payload_out, "pong2", 5) == 0);

    /* Both must be completed and in transport mode */
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(client));
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(server));
    ASSERT_EQ(CAIS_NOISE_ACTION_SPLIT, cais_noise_get_action(client));
    ASSERT_EQ(CAIS_NOISE_ACTION_SPLIT, cais_noise_get_action(server));

    /* Transport phase: Client to Server */
    ASSERT_EQ(CAIS_OK, cais_noise_encrypt(client, "Hello Secure Noise!", 19, cipher, sizeof(cipher), &clen, err, sizeof(err)));
    ASSERT_EQ(19 + CAIS_NOISE_TAG_LEN, clen);

    ASSERT_EQ(CAIS_OK, cais_noise_decrypt(server, cipher, clen, plain, sizeof(plain), &plen, err, sizeof(err)));
    ASSERT_EQ(19, plen);
    plain[plen] = '\0';
    ASSERT_STR_EQ("Hello Secure Noise!", (char *)plain);

    /* Transport phase: Server to Client */
    ASSERT_EQ(CAIS_OK, cais_noise_encrypt(server, "Reply from Server", 17, cipher, sizeof(cipher), &clen, err, sizeof(err)));
    ASSERT_EQ(17 + CAIS_NOISE_TAG_LEN, clen);

    ASSERT_EQ(CAIS_OK, cais_noise_decrypt(client, cipher, clen, plain, sizeof(plain), &plen, err, sizeof(err)));
    ASSERT_EQ(17, plen);
    plain[plen] = '\0';
    ASSERT_STR_EQ("Reply from Server", (char *)plain);

    cais_noise_free(client);
    cais_noise_free(server);
    PASS();
}

TEST test_noise_xx_handshake(void)
{
    cais_noise_t *client;
    cais_noise_t *server;
    char err[128];

    unsigned char client_priv[32], client_pub[32];
    unsigned char server_priv[32], server_pub[32];

    unsigned char msg1[128], msg2[128], msg3[128];
    unsigned char payload[128];
    size_t m1_len = 0, m2_len = 0, m3_len = 0, plen = 0;

    unsigned char cipher[128], plain[128];
    size_t clen = 0;

    /* Generate static identities */
    ASSERT_EQ(CAIS_OK, cais_noise_keygen(client_priv, client_pub, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_keygen(server_priv, server_pub, err, sizeof(err)));

    client = cais_noise_create(CAIS_NOISE_XX, CAIS_NOISE_INITIATOR, err, sizeof(err));
    server = cais_noise_create(CAIS_NOISE_XX, CAIS_NOISE_RESPONDER, err, sizeof(err));
    ASSERT(client != NULL);
    ASSERT(server != NULL);

    ASSERT_EQ(CAIS_OK, cais_noise_set_static_key(client, client_priv, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_set_static_key(server, server_priv, err, sizeof(err)));

    ASSERT_EQ(CAIS_OK, cais_noise_start(client, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_start(server, err, sizeof(err)));

    /* Message 1 (-> e): Client writes -> Server reads */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(client, "init_data", 9, msg1, sizeof(msg1), &m1_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(server, msg1, m1_len, payload, sizeof(payload), &plen, err, sizeof(err)));
    ASSERT_EQ(9, plen);
    ASSERT(memcmp(payload, "init_data", 9) == 0);

    /* Message 2 (<- e, ee, s, es): Server writes -> Client reads */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(server, "resp_data", 9, msg2, sizeof(msg2), &m2_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(client, msg2, m2_len, payload, sizeof(payload), &plen, err, sizeof(err)));
    ASSERT_EQ(9, plen);
    ASSERT(memcmp(payload, "resp_data", 9) == 0);

    /* Message 3 (-> s, se): Client writes -> Server reads */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(client, "done_data", 9, msg3, sizeof(msg3), &m3_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(server, msg3, m3_len, payload, sizeof(payload), &plen, err, sizeof(err)));
    ASSERT_EQ(9, plen);
    ASSERT(memcmp(payload, "done_data", 9) == 0);

    /* Verify mutual authentication: both sides know each other's static public key */
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(client));
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(server));

    ASSERT(cais_noise_get_remote_static(client) != NULL);
    ASSERT(cais_noise_get_remote_static(server) != NULL);

    ASSERT(memcmp(cais_noise_get_remote_static(client), server_pub, 32) == 0);
    ASSERT(memcmp(cais_noise_get_remote_static(server), client_pub, 32) == 0);

    /* Transport Phase bidirectional communication */
    ASSERT_EQ(CAIS_OK, cais_noise_encrypt(client, "Authenticated Client Payload", 28, cipher, sizeof(cipher), &clen, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_decrypt(server, cipher, clen, plain, sizeof(plain), &plen, err, sizeof(err)));
    ASSERT_EQ(28, plen);
    plain[plen] = '\0';
    ASSERT_STR_EQ("Authenticated Client Payload", (char *)plain);

    cais_noise_free(client);
    cais_noise_free(server);
    PASS();
}

TEST test_noise_nk_handshake(void)
{
    cais_noise_t *client;
    cais_noise_t *server;
    char err[128];

    unsigned char server_priv[32], server_pub[32];
    unsigned char msg1[128], msg2[128];
    unsigned char payload[128];
    size_t m1_len = 0, m2_len = 0, plen = 0;

    unsigned char cipher[128], plain[128];
    size_t clen = 0;

    /* Server generates static identity */
    ASSERT_EQ(CAIS_OK, cais_noise_keygen(server_priv, server_pub, err, sizeof(err)));

    client = cais_noise_create(CAIS_NOISE_NK, CAIS_NOISE_INITIATOR, err, sizeof(err));
    server = cais_noise_create(CAIS_NOISE_NK, CAIS_NOISE_RESPONDER, err, sizeof(err));
    ASSERT(client != NULL);
    ASSERT(server != NULL);

    /* Client configures known server public key; Server configures static private key */
    ASSERT_EQ(CAIS_OK, cais_noise_set_remote_key(client, server_pub, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_set_static_key(server, server_priv, err, sizeof(err)));

    ASSERT_EQ(CAIS_OK, cais_noise_start(client, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_start(server, err, sizeof(err)));

    /* Message 1 (-> e, es): Client writes -> Server reads */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(client, "nk_req", 6, msg1, sizeof(msg1), &m1_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(server, msg1, m1_len, payload, sizeof(payload), &plen, err, sizeof(err)));
    ASSERT_EQ(6, plen);
    ASSERT(memcmp(payload, "nk_req", 6) == 0);

    /* Message 2 (<- e, ee): Server writes -> Client reads */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(server, "nk_resp", 7, msg2, sizeof(msg2), &m2_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(client, msg2, m2_len, payload, sizeof(payload), &plen, err, sizeof(err)));
    ASSERT_EQ(7, plen);
    ASSERT(memcmp(payload, "nk_resp", 7) == 0);

    /* Handshake complete after 1 RTT (2 messages) */
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(client));
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(server));

    /* Transport Phase */
    ASSERT_EQ(CAIS_OK, cais_noise_encrypt(client, "NK Anonymous Client Query", 25, cipher, sizeof(cipher), &clen, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_decrypt(server, cipher, clen, plain, sizeof(plain), &plen, err, sizeof(err)));
    ASSERT_EQ(25, plen);
    plain[plen] = '\0';
    ASSERT_STR_EQ("NK Anonymous Client Query", (char *)plain);

    cais_noise_free(client);
    cais_noise_free(server);
    PASS();
}

TEST test_noise_ik_handshake(void)
{
    cais_noise_t *client;
    cais_noise_t *server;
    char err[128];

    unsigned char client_priv[32], client_pub[32];
    unsigned char server_priv[32], server_pub[32];

    unsigned char msg1[128], msg2[128];
    unsigned char payload[128];
    size_t m1_len = 0, m2_len = 0, plen = 0;

    unsigned char cipher[128], plain[128];
    size_t clen = 0;

    /* Both parties have static keypairs */
    ASSERT_EQ(CAIS_OK, cais_noise_keygen(client_priv, client_pub, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_keygen(server_priv, server_pub, err, sizeof(err)));

    client = cais_noise_create(CAIS_NOISE_IK, CAIS_NOISE_INITIATOR, err, sizeof(err));
    server = cais_noise_create(CAIS_NOISE_IK, CAIS_NOISE_RESPONDER, err, sizeof(err));
    ASSERT(client != NULL);
    ASSERT(server != NULL);

    /* Client has static key + knows server key; Server has static key */
    ASSERT_EQ(CAIS_OK, cais_noise_set_static_key(client, client_priv, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_set_remote_key(client, server_pub, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_set_static_key(server, server_priv, err, sizeof(err)));

    ASSERT_EQ(CAIS_OK, cais_noise_start(client, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_start(server, err, sizeof(err)));

    /* Message 1 (-> e, es, s, ss): Client writes -> Server reads */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(client, "ik_init", 7, msg1, sizeof(msg1), &m1_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(server, msg1, m1_len, payload, sizeof(payload), &plen, err, sizeof(err)));
    ASSERT_EQ(7, plen);
    ASSERT(memcmp(payload, "ik_init", 7) == 0);

    /* Server learned client static identity from Message 1 */
    ASSERT(cais_noise_get_remote_static(server) != NULL);
    ASSERT(memcmp(cais_noise_get_remote_static(server), client_pub, 32) == 0);

    /* Message 2 (<- e, ee, se): Server writes -> Client reads */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(server, "ik_reply", 8, msg2, sizeof(msg2), &m2_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(client, msg2, m2_len, payload, sizeof(payload), &plen, err, sizeof(err)));
    ASSERT_EQ(8, plen);
    ASSERT(memcmp(payload, "ik_reply", 8) == 0);

    /* Handshake complete after 1 RTT (WireGuard pattern) */
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(client));
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(server));

    /* Transport Phase */
    ASSERT_EQ(CAIS_OK, cais_noise_encrypt(client, "1-RTT WireGuard Style Tunnel", 28, cipher, sizeof(cipher), &clen, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_decrypt(server, cipher, clen, plain, sizeof(plain), &plen, err, sizeof(err)));
    ASSERT_EQ(28, plen);
    plain[plen] = '\0';
    ASSERT_STR_EQ("1-RTT WireGuard Style Tunnel", (char *)plain);

    cais_noise_free(client);
    cais_noise_free(server);
    PASS();
}

TEST test_noise_tamper_detection(void)
{
    cais_noise_t *client;
    cais_noise_t *server;
    char err[128];
    unsigned char msg1[128], msg2[128];
    size_t m1_len, m2_len, plen;
    unsigned char cipher[128], plain[128];
    size_t clen;

    client = cais_noise_create(CAIS_NOISE_NN, CAIS_NOISE_INITIATOR, err, sizeof(err));
    server = cais_noise_create(CAIS_NOISE_NN, CAIS_NOISE_RESPONDER, err, sizeof(err));
    cais_noise_start(client, err, sizeof(err));
    cais_noise_start(server, err, sizeof(err));

    cais_noise_write_message(client, NULL, 0, msg1, sizeof(msg1), &m1_len, err, sizeof(err));
    cais_noise_read_message(server, msg1, m1_len, plain, sizeof(plain), &plen, err, sizeof(err));
    cais_noise_write_message(server, NULL, 0, msg2, sizeof(msg2), &m2_len, err, sizeof(err));
    cais_noise_read_message(client, msg2, m2_len, plain, sizeof(plain), &plen, err, sizeof(err));

    /* Encrypt valid message */
    ASSERT_EQ(CAIS_OK, cais_noise_encrypt(client, "Top Secret Data", 15, cipher, sizeof(cipher), &clen, err, sizeof(err)));

    /* Tamper 1 bit in the ciphertext */
    cipher[5] ^= 0x01;

    /* Decryption MUST fail with authentication error */
    ASSERT_EQ(CAIS_ERROR, cais_noise_decrypt(server, cipher, clen, plain, sizeof(plain), &plen, err, sizeof(err)));

    cais_noise_free(client);
    cais_noise_free(server);
    PASS();
}

TEST test_noise_socket_framed_transfer(void)
{
    char err[128];
    int port = 19940;
    cais_socket_t *listener;
    cais_socket_t *client_sock;
    cais_socket_t *server_sock;
    char client_ip[64];
    int client_port = 0;

    cais_noise_t *client;
    cais_noise_t *server;
    unsigned char c_priv[32], c_pub[32];
    unsigned char s_priv[32], s_pub[32];

    unsigned char m1[128], m2[128], m3[128];
    unsigned char pld[128];
    size_t m1_len, m2_len, m3_len, plen;
    size_t sent, recvd;
    char recv_buf[128];

    /* 1. Setup TCP Loopback Connection */
    listener = cais_socket_bind("127.0.0.1", port, 5, CAIS_SOCK_BLOCKING, err, sizeof(err));
    if (!listener) {
        port = 19941;
        listener = cais_socket_bind("127.0.0.1", port, 5, CAIS_SOCK_BLOCKING, err, sizeof(err));
    }
    ASSERT(listener != NULL);

    client_sock = cais_socket_connect("127.0.0.1", port, CAIS_SOCK_BLOCKING, err, sizeof(err));
    ASSERT(client_sock != NULL);

    server_sock = cais_socket_accept(listener, client_ip, sizeof(client_ip), &client_port, err, sizeof(err));
    ASSERT(server_sock != NULL);

    /* 2. Setup Noise_XX Sessions */
    cais_noise_keygen(c_priv, c_pub, err, sizeof(err));
    cais_noise_keygen(s_priv, s_pub, err, sizeof(err));

    client = cais_noise_create(CAIS_NOISE_XX, CAIS_NOISE_INITIATOR, err, sizeof(err));
    server = cais_noise_create(CAIS_NOISE_XX, CAIS_NOISE_RESPONDER, err, sizeof(err));
    cais_noise_set_static_key(client, c_priv, err, sizeof(err));
    cais_noise_set_static_key(server, s_priv, err, sizeof(err));
    cais_noise_start(client, err, sizeof(err));
    cais_noise_start(server, err, sizeof(err));

    /* 3. Stepwise Handshake exchange over TCP sockets */
    /* Msg 1: Client -> Server */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(client, NULL, 0, m1, sizeof(m1), &m1_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_SOCK_OK, cais_socket_send(client_sock, (char *)m1, m1_len, &sent));
    ASSERT_EQ(CAIS_SOCK_OK, cais_socket_recv(server_sock, (char *)m1, m1_len, &recvd));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(server, m1, m1_len, pld, sizeof(pld), &plen, err, sizeof(err)));

    /* Msg 2: Server -> Client */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(server, NULL, 0, m2, sizeof(m2), &m2_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_SOCK_OK, cais_socket_send(server_sock, (char *)m2, m2_len, &sent));
    ASSERT_EQ(CAIS_SOCK_OK, cais_socket_recv(client_sock, (char *)m2, m2_len, &recvd));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(client, m2, m2_len, pld, sizeof(pld), &plen, err, sizeof(err)));

    /* Msg 3: Client -> Server */
    ASSERT_EQ(CAIS_OK, cais_noise_write_message(client, NULL, 0, m3, sizeof(m3), &m3_len, err, sizeof(err)));
    ASSERT_EQ(CAIS_SOCK_OK, cais_socket_send(client_sock, (char *)m3, m3_len, &sent));
    ASSERT_EQ(CAIS_SOCK_OK, cais_socket_recv(server_sock, (char *)m3, m3_len, &recvd));
    ASSERT_EQ(CAIS_OK, cais_noise_read_message(server, m3, m3_len, pld, sizeof(pld), &plen, err, sizeof(err)));

    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(client));
    ASSERT_EQ(CAIS_TRUE, cais_noise_is_complete(server));

    /* 4. Length-prefixed framed transport over connected TCP sockets */
    /* Client sends encrypted message to server */
    ASSERT_EQ(CAIS_OK, cais_noise_send_socket(client, client_sock, "Secure Socket Message!", 22, &sent, err, sizeof(err)));
    ASSERT_EQ(22, sent);

    /* Server receives and decrypts message */
    ASSERT_EQ(CAIS_OK, cais_noise_recv_socket(server, server_sock, recv_buf, sizeof(recv_buf), &recvd, err, sizeof(err)));
    ASSERT_EQ(22, recvd);
    recv_buf[recvd] = '\0';
    ASSERT_STR_EQ("Secure Socket Message!", recv_buf);

    /* Server replies to client */
    ASSERT_EQ(CAIS_OK, cais_noise_send_socket(server, server_sock, "Secure Response!", 16, &sent, err, sizeof(err)));
    ASSERT_EQ(16, sent);

    ASSERT_EQ(CAIS_OK, cais_noise_recv_socket(client, client_sock, recv_buf, sizeof(recv_buf), &recvd, err, sizeof(err)));
    ASSERT_EQ(16, recvd);
    recv_buf[recvd] = '\0';
    ASSERT_STR_EQ("Secure Response!", recv_buf);

    /* Cleanup */
    cais_noise_free(client);
    cais_noise_free(server);
    cais_socket_close(client_sock);
    cais_socket_close(server_sock);
    cais_socket_close(listener);
    PASS();
}

static size_t hex_to_bin(const char *hex, unsigned char *out, size_t max_out)
{
    size_t i, len;
    if (!hex) return 0;
    len = strlen(hex);
    if (len % 2 != 0 || len / 2 > max_out) return 0;
    for (i = 0; i < len; i += 2) {
        unsigned int byte_val;
        if (sscanf(hex + i, "%02x", &byte_val) != 1) return 0;
        out[i / 2] = (unsigned char)byte_val;
    }
    return len / 2;
}

static JSON_Object *find_cacophony_vector(JSON_Value *root, const char *proto_name)
{
    JSON_Array *vectors;
    size_t i, count;
    if (!root) return NULL;
    vectors = json_object_get_array(json_value_get_object(root), "vectors");
    if (!vectors) return NULL;
    count = json_array_get_count(vectors);
    for (i = 0; i < count; i++) {
        JSON_Object *vec = json_array_get_object(vectors, i);
        const char *name = json_object_get_string(vec, "protocol_name");
        if (name && strcmp(name, proto_name) == 0) {
            return vec;
        }
    }
    return NULL;
}

static int run_cacophony_vector(JSON_Object *vec)
{
    const char *proto;
    cais_noise_pattern_t pattern;
    cais_noise_t *client = NULL;
    cais_noise_t *server = NULL;
    char err[128];
    const char *s;
    unsigned char key_buf[32];
    unsigned char pro_buf[256];
    size_t pro_len;
    JSON_Array *msgs;
    size_t m_count, m;
    int handshake_verified = 0;
    int transport_sender = 0; /* 0 = initiator (client), 1 = responder (server) */

    if (!vec) return 0;
    proto = json_object_get_string(vec, "protocol_name");
    if (!proto) return 0;

    if (strstr(proto, "Noise_NN_")) pattern = CAIS_NOISE_NN;
    else if (strstr(proto, "Noise_NK_")) pattern = CAIS_NOISE_NK;
    else if (strstr(proto, "Noise_IK_")) pattern = CAIS_NOISE_IK;
    else if (strstr(proto, "Noise_XX_")) pattern = CAIS_NOISE_XX;
    else return 0;

    client = cais_noise_create(pattern, CAIS_NOISE_INITIATOR, err, sizeof(err));
    server = cais_noise_create(pattern, CAIS_NOISE_RESPONDER, err, sizeof(err));
    if (!client || !server) {
        if (client) cais_noise_free(client);
        if (server) cais_noise_free(server);
        return 0;
    }

    s = json_object_get_string(vec, "init_static");
    if (s && hex_to_bin(s, key_buf, 32) == 32) {
        if (cais_noise_set_static_key(client, key_buf, err, sizeof(err)) != CAIS_OK) goto fail;
    }
    s = json_object_get_string(vec, "resp_static");
    if (s && hex_to_bin(s, key_buf, 32) == 32) {
        if (cais_noise_set_static_key(server, key_buf, err, sizeof(err)) != CAIS_OK) goto fail;
    }
    s = json_object_get_string(vec, "init_remote_static");
    if (s && hex_to_bin(s, key_buf, 32) == 32) {
        if (cais_noise_set_remote_key(client, key_buf, err, sizeof(err)) != CAIS_OK) goto fail;
    }
    s = json_object_get_string(vec, "resp_remote_static");
    if (s && hex_to_bin(s, key_buf, 32) == 32) {
        if (cais_noise_set_remote_key(server, key_buf, err, sizeof(err)) != CAIS_OK) goto fail;
    }
    s = json_object_get_string(vec, "init_ephemeral");
    if (s && hex_to_bin(s, key_buf, 32) == 32) {
        if (cais_noise_set_ephemeral_key(client, key_buf, err, sizeof(err)) != CAIS_OK) goto fail;
    }
    s = json_object_get_string(vec, "resp_ephemeral");
    if (s && hex_to_bin(s, key_buf, 32) == 32) {
        if (cais_noise_set_ephemeral_key(server, key_buf, err, sizeof(err)) != CAIS_OK) goto fail;
    }
    s = json_object_get_string(vec, "init_prologue");
    if (s) {
        pro_len = hex_to_bin(s, pro_buf, sizeof(pro_buf));
        if (cais_noise_set_prologue(client, pro_buf, pro_len, err, sizeof(err)) != CAIS_OK) goto fail;
    }
    s = json_object_get_string(vec, "resp_prologue");
    if (s) {
        pro_len = hex_to_bin(s, pro_buf, sizeof(pro_buf));
        if (cais_noise_set_prologue(server, pro_buf, pro_len, err, sizeof(err)) != CAIS_OK) goto fail;
    }

    if (cais_noise_start(client, err, sizeof(err)) != CAIS_OK) goto fail;
    if (cais_noise_start(server, err, sizeof(err)) != CAIS_OK) goto fail;

    msgs = json_object_get_array(vec, "messages");
    if (!msgs) goto fail;
    m_count = json_array_get_count(msgs);

    for (m = 0; m < m_count; m++) {
        JSON_Object *msg_obj = json_array_get_object(msgs, m);
        const char *hex_pld = json_object_get_string(msg_obj, "payload");
        const char *hex_cip = json_object_get_string(msg_obj, "ciphertext");
        unsigned char exp_pld[512], exp_cip[512], actual_cip[512], actual_pld[512];
        size_t exp_pld_len = hex_to_bin(hex_pld, exp_pld, sizeof(exp_pld));
        size_t exp_cip_len = hex_to_bin(hex_cip, exp_cip, sizeof(exp_cip));
        size_t act_cip_len = 0, act_pld_len = 0;

        if (!cais_noise_is_complete(client)) {
            /* Handshake step */
            if (cais_noise_get_action(client) == CAIS_NOISE_ACTION_WRITE_MSG) {
                /* Initiator -> Responder */
                if (cais_noise_write_message(client, exp_pld, exp_pld_len, actual_cip, sizeof(actual_cip), &act_cip_len, err, sizeof(err)) != CAIS_OK) goto fail;
                if (act_cip_len != exp_cip_len || memcmp(actual_cip, exp_cip, act_cip_len) != 0) goto fail;
                if (cais_noise_read_message(server, actual_cip, act_cip_len, actual_pld, sizeof(actual_pld), &act_pld_len, err, sizeof(err)) != CAIS_OK) goto fail;
                if (act_pld_len != exp_pld_len || memcmp(actual_pld, exp_pld, act_pld_len) != 0) goto fail;
                transport_sender = 1;
            } else {
                /* Responder -> Initiator */
                if (cais_noise_write_message(server, exp_pld, exp_pld_len, actual_cip, sizeof(actual_cip), &act_cip_len, err, sizeof(err)) != CAIS_OK) goto fail;
                if (act_cip_len != exp_cip_len || memcmp(actual_cip, exp_cip, act_cip_len) != 0) goto fail;
                if (cais_noise_read_message(client, actual_cip, act_cip_len, actual_pld, sizeof(actual_pld), &act_pld_len, err, sizeof(err)) != CAIS_OK) goto fail;
                if (act_pld_len != exp_pld_len || memcmp(actual_pld, exp_pld, act_pld_len) != 0) goto fail;
                transport_sender = 0;
            }

            if (cais_noise_is_complete(client) && !handshake_verified) {
                const char *hex_hash = json_object_get_string(vec, "handshake_hash");
                unsigned char exp_h[32], act_h_client[32], act_h_server[32];
                if (!hex_hash || hex_to_bin(hex_hash, exp_h, 32) != 32) goto fail;
                if (cais_noise_get_handshake_hash(client, act_h_client, err, sizeof(err)) != CAIS_OK) goto fail;
                if (cais_noise_get_handshake_hash(server, act_h_server, err, sizeof(err)) != CAIS_OK) goto fail;
                if (memcmp(act_h_client, exp_h, 32) != 0 || memcmp(act_h_server, exp_h, 32) != 0) goto fail;
                handshake_verified = 1;
            }
        } else {
            /* Transport message (post-handshake split) */
            if (transport_sender == 0) {
                /* Initiator -> Responder */
                if (cais_noise_encrypt(client, exp_pld, exp_pld_len, actual_cip, sizeof(actual_cip), &act_cip_len, err, sizeof(err)) != CAIS_OK) goto fail;
                if (act_cip_len != exp_cip_len || memcmp(actual_cip, exp_cip, act_cip_len) != 0) goto fail;
                if (cais_noise_decrypt(server, actual_cip, act_cip_len, actual_pld, sizeof(actual_pld), &act_pld_len, err, sizeof(err)) != CAIS_OK) goto fail;
                if (act_pld_len != exp_pld_len || memcmp(actual_pld, exp_pld, act_pld_len) != 0) goto fail;
                transport_sender = 1;
            } else {
                /* Responder -> Initiator */
                if (cais_noise_encrypt(server, exp_pld, exp_pld_len, actual_cip, sizeof(actual_cip), &act_cip_len, err, sizeof(err)) != CAIS_OK) goto fail;
                if (act_cip_len != exp_cip_len || memcmp(actual_cip, exp_cip, act_cip_len) != 0) goto fail;
                if (cais_noise_decrypt(client, actual_cip, act_cip_len, actual_pld, sizeof(actual_pld), &act_pld_len, err, sizeof(err)) != CAIS_OK) goto fail;
                if (act_pld_len != exp_pld_len || memcmp(actual_pld, exp_pld, act_pld_len) != 0) goto fail;
                transport_sender = 0;
            }
        }
    }

    cais_noise_free(client);
    cais_noise_free(server);
    return 1;

fail:
    if (client) cais_noise_free(client);
    if (server) cais_noise_free(server);
    return 0;
}

TEST test_noise_vector_cacophony_nn(void)
{
    JSON_Value *root = json_parse_file("tests/vectors/cacophony.json");
    JSON_Object *vec;
    ASSERT(root != NULL);
    vec = find_cacophony_vector(root, "Noise_NN_25519_ChaChaPoly_BLAKE2s");
    ASSERT(vec != NULL);
    ASSERT_EQ(1, run_cacophony_vector(vec));
    json_value_free(root);
    PASS();
}

TEST test_noise_vector_cacophony_nk(void)
{
    JSON_Value *root = json_parse_file("tests/vectors/cacophony.json");
    JSON_Object *vec;
    ASSERT(root != NULL);
    vec = find_cacophony_vector(root, "Noise_NK_25519_ChaChaPoly_BLAKE2s");
    ASSERT(vec != NULL);
    ASSERT_EQ(1, run_cacophony_vector(vec));
    json_value_free(root);
    PASS();
}

TEST test_noise_vector_cacophony_ik(void)
{
    JSON_Value *root = json_parse_file("tests/vectors/cacophony.json");
    JSON_Object *vec;
    ASSERT(root != NULL);
    vec = find_cacophony_vector(root, "Noise_IK_25519_ChaChaPoly_BLAKE2s");
    ASSERT(vec != NULL);
    ASSERT_EQ(1, run_cacophony_vector(vec));
    json_value_free(root);
    PASS();
}

TEST test_noise_vector_cacophony_xx(void)
{
    JSON_Value *root = json_parse_file("tests/vectors/cacophony.json");
    JSON_Object *vec;
    ASSERT(root != NULL);
    vec = find_cacophony_vector(root, "Noise_XX_25519_ChaChaPoly_BLAKE2s");
    ASSERT(vec != NULL);
    ASSERT_EQ(1, run_cacophony_vector(vec));
    json_value_free(root);
    PASS();
}

TEST test_noise_empty_prologue_hash(void)
{
    char err[128];
    unsigned char act_h[32];
    /*
     * For Noise_NN_25519_ChaChaPoly_BLAKE2s:
     * pname length is 33 bytes (> 32), so:
     *   h0 = BLAKE2s("Noise_NN_25519_ChaChaPoly_BLAKE2s")
     * Section 5.3: MixHash(prologue) must run even when prologue is empty (0 bytes):
     *   h1 = BLAKE2s(h0 || "") = 00b6f1c6799546c330d32d638953a16e33dfc15b22450efb33d28cab6b041599
     */
    static const unsigned char exp_nn_h[32] = {
        0x00, 0xb6, 0xf1, 0xc6, 0x79, 0x95, 0x46, 0xc3,
        0x30, 0xd3, 0x2d, 0x63, 0x89, 0x53, 0xa1, 0x6e,
        0x33, 0xdf, 0xc1, 0x5b, 0x22, 0x45, 0x0e, 0xfb,
        0x33, 0xd2, 0x8c, 0xab, 0x6b, 0x04, 0x15, 0x99
    };
    cais_noise_t *client = cais_noise_create(CAIS_NOISE_NN, CAIS_NOISE_INITIATOR, err, sizeof(err));
    ASSERT(client != NULL);
    ASSERT_EQ(CAIS_OK, cais_noise_start(client, err, sizeof(err)));
    ASSERT_EQ(CAIS_OK, cais_noise_get_handshake_hash(client, act_h, err, sizeof(err)));
    ASSERT(memcmp(act_h, exp_nn_h, 32) == 0);
    cais_noise_free(client);
    PASS();
}

SUITE(noise_suite)
{
    RUN_TEST(test_noise_keygen);
    RUN_TEST(test_noise_nn_handshake);
    RUN_TEST(test_noise_empty_prologue_hash);
    RUN_TEST(test_noise_xx_handshake);
    RUN_TEST(test_noise_nk_handshake);
    RUN_TEST(test_noise_ik_handshake);
    RUN_TEST(test_noise_tamper_detection);
    RUN_TEST(test_noise_socket_framed_transfer);
    RUN_TEST(test_noise_vector_cacophony_nn);
    RUN_TEST(test_noise_vector_cacophony_nk);
    RUN_TEST(test_noise_vector_cacophony_ik);
    RUN_TEST(test_noise_vector_cacophony_xx);
}
