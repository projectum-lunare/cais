/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_NOISE_H
#define CAIS_NOISE_H

#include <stddef.h>
#include <cais/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct cais_socket;
struct parallel_port;
struct serial_port;

/*
 * =============================================================================
 * Constants
 * =============================================================================
 */
#define CAIS_NOISE_KEY_LEN     32
#define CAIS_NOISE_HASH_LEN    32
#define CAIS_NOISE_TAG_LEN     16
#define CAIS_NOISE_MAX_MSG_LEN 65535

/*
 * =============================================================================
 * Enumerations
 * =============================================================================
 */

/* Noise Handshake Patterns */
typedef enum cais_noise_pattern {
    CAIS_NOISE_XX = 0, /* Mutual auth, identity hiding, no pre-shared keys */
    CAIS_NOISE_NN = 1, /* Ephemeral only, opportunistic encryption (no auth) */
    CAIS_NOISE_NK = 2, /* Anonymous client, known responder public key */
    CAIS_NOISE_IK = 3  /* 1-RTT, initiator identity known to responder */
} cais_noise_pattern_t;

/* Noise Handshake Role */
typedef enum cais_noise_role {
    CAIS_NOISE_INITIATOR = 0, /* Local party initiates the handshake */
    CAIS_NOISE_RESPONDER = 1  /* Local party waits for the first message */
} cais_noise_role_t;

/* Handshake Next Action */
typedef enum cais_noise_action {
    CAIS_NOISE_ACTION_WRITE_MSG = 1, /* Local must write the next handshake message */
    CAIS_NOISE_ACTION_READ_MSG  = 2, /* Local must read the next handshake message */
    CAIS_NOISE_ACTION_SPLIT     = 3, /* Handshake complete: ready for transport */
    CAIS_NOISE_ACTION_FAILED    = -1 /* Handshake failed or protocol violated */
} cais_noise_action_t;

/* Opaque Session Handle */
typedef struct cais_noise cais_noise_t;

/*
 * =============================================================================
 * Key Generation & Utilities
 * =============================================================================
 */

/*
 * Generates an X25519 keypair using the platform CSPRNG.
 * Outputs 32-byte private key and 32-byte public key.
 */
cais_status_t cais_noise_keygen(unsigned char priv[CAIS_NOISE_KEY_LEN],
                                unsigned char pub[CAIS_NOISE_KEY_LEN],
                                char *err_buf, size_t err_cap);

/*
 * Injects external entropy seed for platforms without native CSPRNG (e.g. DOS/Amiga).
 */
cais_status_t cais_noise_seed(const void *entropy, size_t len);

/*
 * =============================================================================
 * Session Lifecycle & Configuration
 * =============================================================================
 */

/*
 * Creates a new Noise session for the specified pattern and role.
 */
cais_noise_t *cais_noise_create(cais_noise_pattern_t pattern,
                                cais_noise_role_t role,
                                char *err_buf, size_t err_cap);

/*
 * Frees a Noise session and securely clears sensitive key material.
 */
void cais_noise_free(cais_noise_t *noise);

/*
 * Configures the local static private key (32 bytes).
 * Derives and stores the corresponding local public key automatically.
 */
cais_status_t cais_noise_set_static_key(cais_noise_t *noise,
                                        const unsigned char priv[CAIS_NOISE_KEY_LEN],
                                        char *err_buf, size_t err_cap);

/*
 * Configures the remote party's known public key (32 bytes).
 * Required for patterns NK and IK (initiator).
 */
cais_status_t cais_noise_set_remote_key(cais_noise_t *noise,
                                        const unsigned char pub[CAIS_NOISE_KEY_LEN],
                                        char *err_buf, size_t err_cap);

/*
 * Injects a fixed ephemeral private key (32 bytes) for deterministic testing against test vectors.
 */
cais_status_t cais_noise_set_ephemeral_key(cais_noise_t *noise,
                                           const unsigned char priv[CAIS_NOISE_KEY_LEN],
                                           char *err_buf, size_t err_cap);

/*
 * Retrieves the current 32-byte handshake hash (h) for channel binding or vector verification.
 */
cais_status_t cais_noise_get_handshake_hash(const cais_noise_t *noise,
                                            unsigned char out_hash[CAIS_NOISE_HASH_LEN],
                                            char *err_buf, size_t err_cap);

/*
 * Configures an optional handshake prologue data block.
 */
cais_status_t cais_noise_set_prologue(cais_noise_t *noise,
                                      const void *prologue, size_t len,
                                      char *err_buf, size_t err_cap);

/*
 * Finalizes configuration and prepares the state machine for the handshake.
 */
cais_status_t cais_noise_start(cais_noise_t *noise,
                               char *err_buf, size_t err_cap);

/*
 * =============================================================================
 * Transport-Agnostic Handshake Step Engine
 * =============================================================================
 */

/*
 * Returns the current required action for the session.
 */
cais_noise_action_t cais_noise_get_action(const cais_noise_t *noise);

/*
 * Writes the next handshake message into out_msg.
 * Optionally appends encrypted payload bytes.
 */
cais_status_t cais_noise_write_message(cais_noise_t *noise,
                                       const void *payload, size_t payload_len,
                                       void *out_msg, size_t out_cap, size_t *out_len,
                                       char *err_buf, size_t err_cap);

/*
 * Reads and processes an incoming handshake message from in_msg.
 * Decrypts and outputs any payload bytes into out_payload.
 */
cais_status_t cais_noise_read_message(cais_noise_t *noise,
                                      const void *in_msg, size_t in_len,
                                      void *out_payload, size_t out_cap, size_t *out_payload_len,
                                      char *err_buf, size_t err_cap);

/*
 * Returns CAIS_TRUE if the handshake has completed successfully and split into transport ciphers.
 */
cais_bool_t cais_noise_is_complete(const cais_noise_t *noise);

/*
 * Retrieves the authenticated remote static public key (32 bytes) if established, or NULL.
 */
const unsigned char *cais_noise_get_remote_static(const cais_noise_t *noise);

/*
 * =============================================================================
 * Transport Phase Encryption / Decryption
 * =============================================================================
 */

/*
 * Encrypts a transport payload using the outbound CipherState.
 * Output size is plain_len + CAIS_NOISE_TAG_LEN (16 bytes).
 */
cais_status_t cais_noise_encrypt(cais_noise_t *noise,
                                 const void *plain, size_t plain_len,
                                 void *out_cipher, size_t out_cap, size_t *out_len,
                                 char *err_buf, size_t err_cap);

/*
 * Decrypts a transport ciphertext using the inbound CipherState.
 * Output size is cipher_len - CAIS_NOISE_TAG_LEN (16 bytes).
 */
cais_status_t cais_noise_decrypt(cais_noise_t *noise,
                                 const void *cipher, size_t cipher_len,
                                 void *out_plain, size_t out_cap, size_t *out_len,
                                 char *err_buf, size_t err_cap);

/*
 * =============================================================================
 * High-Level Stream Wrappers (Framed TCP Socket & Serial)
 * =============================================================================
 */

/*
 * Executes the complete handshake over a connected cais_socket_t.
 */
cais_status_t cais_noise_handshake_socket(cais_noise_t *noise,
                                          struct cais_socket *sock,
                                          char *err_buf, size_t err_cap);

/*
 * Sends a length-prefixed, Noise-encrypted message over a cais_socket_t.
 */
cais_status_t cais_noise_send_socket(cais_noise_t *noise,
                                     struct cais_socket *sock,
                                     const void *data, size_t len, size_t *sent,
                                     char *err_buf, size_t err_cap);

/*
 * Receives and decrypts a length-prefixed Noise message from a cais_socket_t.
 */
cais_status_t cais_noise_recv_socket(cais_noise_t *noise,
                                     struct cais_socket *sock,
                                     void *buf, size_t cap, size_t *received,
                                     char *err_buf, size_t err_cap);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_NOISE_H */
