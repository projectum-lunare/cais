/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_NOISE_INTERNAL_H
#define CAIS_NOISE_INTERNAL_H

#include <cais/cais.h>
#include <cais/noise.h>

#include <monocypher.h>
#include <blake2.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * Low-Level Cryptographic Primitives (32-bit Suite: X25519 + ChaChaPoly + BLAKE2s)
 * =============================================================================
 */

/* Generates cryptographically secure random bytes */
int cais_noise_random_bytes(void *buf, size_t len);

/* BLAKE2s-256 hash (32-byte digest) */
void cais_noise_blake2s(uint8_t out[32], const void *in, size_t inlen);

/* RFC 2104 HMAC with BLAKE2s */
void cais_noise_hmac_blake2s(uint8_t out[32],
                             const uint8_t *key, size_t keylen,
                             const void *in, size_t inlen);

/* Noise HKDF with 2 outputs */
void cais_noise_hkdf2(uint8_t out1[32], uint8_t out2[32],
                      const uint8_t ck[32],
                      const void *ikm, size_t ikmlen);

/* Noise HKDF with 3 outputs */
void cais_noise_hkdf3(uint8_t out1[32], uint8_t out2[32], uint8_t out3[32],
                      const uint8_t ck[32],
                      const void *ikm, size_t ikmlen);

/* RFC 8439 AEAD ChaCha20-Poly1305 with Noise 96-bit nonce (32-bit zero + 64-bit LE counter) */
int cais_noise_aead_encrypt(uint8_t *out_cipher, uint8_t mac[16],
                            const uint8_t key[32], uint64_t nonce,
                            const uint8_t *ad, size_t adlen,
                            const uint8_t *plain, size_t plainlen);

int cais_noise_aead_decrypt(uint8_t *out_plain,
                            const uint8_t key[32], uint64_t nonce,
                            const uint8_t mac[16],
                            const uint8_t *ad, size_t adlen,
                            const uint8_t *cipher, size_t cipherlen);

/*
 * =============================================================================
 * Noise Protocol State Machine Structures
 * =============================================================================
 */

typedef struct cais_noise_cipher {
    uint8_t  k[32];
    uint64_t n;
    int      has_key;
} cais_noise_cipher_t;

typedef struct cais_noise_sym {
    uint8_t             ck[32];
    uint8_t             h[32];
    cais_noise_cipher_t cipher;
} cais_noise_sym_t;

struct cais_noise {
    cais_noise_pattern_t pattern;
    cais_noise_role_t    role;
    cais_noise_sym_t     sym;

    /* Local keys */
    uint8_t s_priv[32];
    uint8_t s_pub[32];
    int     has_s;

    uint8_t e_priv[32];
    uint8_t e_pub[32];
    int     has_e;

    /* Remote keys */
    uint8_t rs[32];
    int     has_rs;

    uint8_t re[32];
    int     has_re;

    /* Prologue buffer */
    uint8_t prologue[256];
    size_t  prologue_len;

    /* Handshake state */
    int                 step;
    cais_noise_action_t action;
    cais_bool_t         is_complete;

    /* Post-handshake transport ciphers */
    cais_noise_cipher_t tx_cipher;
    cais_noise_cipher_t rx_cipher;
};

/* Noise SymmetricState Operations */
void cais_noise_sym_init(cais_noise_sym_t *sym, const char *protocol_name);
void cais_noise_sym_mix_hash(cais_noise_sym_t *sym, const void *data, size_t len);
void cais_noise_sym_mix_key(cais_noise_sym_t *sym, const void *ikm, size_t ikmlen);
void cais_noise_sym_mix_key_and_hash(cais_noise_sym_t *sym, const void *ikm, size_t ikmlen);
int  cais_noise_sym_encrypt_and_hash(cais_noise_sym_t *sym,
                                     const void *plain, size_t plainlen,
                                     void *out_cipher, size_t *out_len);
int  cais_noise_sym_decrypt_and_hash(cais_noise_sym_t *sym,
                                     const void *cipher, size_t cipherlen,
                                     void *out_plain, size_t *out_len);
void cais_noise_sym_split(cais_noise_sym_t *sym,
                          cais_noise_cipher_t *c1,
                          cais_noise_cipher_t *c2);

/* Error formatting helper */
void cais_noise_set_err(char *err_buf, size_t err_cap, const char *msg);
void cais_noise_set_err_fmt(char *err_buf, size_t err_cap, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_NOISE_INTERNAL_H */
