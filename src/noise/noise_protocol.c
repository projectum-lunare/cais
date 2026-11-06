/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "noise_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * =============================================================================
 * SymmetricState Implementation
 * =============================================================================
 */

void cais_noise_sym_init(cais_noise_sym_t *sym, const char *protocol_name)
{
    size_t len = strlen(protocol_name);

    memset(sym, 0, sizeof(*sym));
    if (len <= 32) {
        memcpy(sym->h, protocol_name, len);
        memset(sym->h + len, 0, 32 - len);
    } else {
        cais_noise_blake2s(sym->h, protocol_name, len);
    }
    memcpy(sym->ck, sym->h, 32);
    sym->cipher.has_key = 0;
    sym->cipher.n = 0;
}

void cais_noise_sym_mix_hash(cais_noise_sym_t *sym, const void *data, size_t len)
{
    blake2s_state S;
    blake2s_init(&S, 32);
    blake2s_update(&S, sym->h, 32);
    if (len > 0 && data != NULL) {
        blake2s_update(&S, data, len);
    }
    blake2s_final(&S, sym->h, 32);
}

void cais_noise_sym_mix_key(cais_noise_sym_t *sym, const void *ikm, size_t ikmlen)
{
    cais_noise_hkdf2(sym->ck, sym->cipher.k, sym->ck, ikm, ikmlen);
    sym->cipher.n = 0;
    sym->cipher.has_key = 1;
}

void cais_noise_sym_mix_key_and_hash(cais_noise_sym_t *sym, const void *ikm, size_t ikmlen)
{
    uint8_t temp_h[32];
    cais_noise_hkdf3(sym->ck, temp_h, sym->cipher.k, sym->ck, ikm, ikmlen);
    cais_noise_sym_mix_hash(sym, temp_h, 32);
    sym->cipher.n = 0;
    sym->cipher.has_key = 1;
    crypto_wipe(temp_h, sizeof(temp_h));
}

int cais_noise_sym_encrypt_and_hash(cais_noise_sym_t *sym,
                                     const void *plain, size_t plainlen,
                                     void *out_cipher, size_t *out_len)
{
    if (sym->cipher.has_key) {
        uint8_t mac[16];
        if (plainlen > 0 && plain != NULL) {
            cais_noise_aead_encrypt((uint8_t *)out_cipher, mac,
                                    sym->cipher.k, sym->cipher.n,
                                    sym->h, 32,
                                    (const uint8_t *)plain, plainlen);
        } else {
            cais_noise_aead_encrypt((uint8_t *)out_cipher, mac,
                                    sym->cipher.k, sym->cipher.n,
                                    sym->h, 32,
                                    NULL, 0);
        }
        memcpy((uint8_t *)out_cipher + plainlen, mac, 16);
        sym->cipher.n++;
        *out_len = plainlen + 16;
        cais_noise_sym_mix_hash(sym, out_cipher, *out_len);
    } else {
        if (plainlen > 0 && plain != NULL) {
            memcpy(out_cipher, plain, plainlen);
        }
        *out_len = plainlen;
        cais_noise_sym_mix_hash(sym, plain, plainlen);
    }
    return 0;
}

int cais_noise_sym_decrypt_and_hash(cais_noise_sym_t *sym,
                                     const void *cipher, size_t cipherlen,
                                     void *out_plain, size_t *out_len)
{
    if (sym->cipher.has_key) {
        uint8_t mac[16];
        size_t plainlen;

        if (cipherlen < 16) {
            return -1;
        }
        plainlen = cipherlen - 16;
        memcpy(mac, (const uint8_t *)cipher + plainlen, 16);

        if (cais_noise_aead_decrypt((uint8_t *)out_plain,
                                    sym->cipher.k, sym->cipher.n,
                                    mac,
                                    sym->h, 32,
                                    (const uint8_t *)cipher, plainlen) != 0) {
            return -1;
        }

        sym->cipher.n++;
        cais_noise_sym_mix_hash(sym, cipher, cipherlen);
        *out_len = plainlen;
    } else {
        if (cipherlen > 0 && cipher != NULL) {
            memcpy(out_plain, cipher, cipherlen);
        }
        *out_len = cipherlen;
        cais_noise_sym_mix_hash(sym, cipher, cipherlen);
    }
    return 0;
}

void cais_noise_sym_split(cais_noise_sym_t *sym,
                          cais_noise_cipher_t *c1,
                          cais_noise_cipher_t *c2)
{
    cais_noise_hkdf2(c1->k, c2->k, sym->ck, NULL, 0);
    c1->n = 0;
    c1->has_key = 1;

    c2->n = 0;
    c2->has_key = 1;
}

/*
 * =============================================================================
 * Session Lifecycle & Configuration
 * =============================================================================
 */

cais_noise_t *cais_noise_create(cais_noise_pattern_t pattern,
                                cais_noise_role_t role,
                                char *err_buf, size_t err_cap)
{
    cais_noise_t *n;

    if (pattern != CAIS_NOISE_XX && pattern != CAIS_NOISE_NN &&
        pattern != CAIS_NOISE_NK && pattern != CAIS_NOISE_IK) {
        cais_noise_set_err(err_buf, err_cap, "currently supported patterns are XX, NN, NK, and IK");
        return NULL;
    }

    n = (cais_noise_t *)malloc(sizeof(cais_noise_t));
    if (n == NULL) {
        cais_noise_set_err(err_buf, err_cap, "out of memory allocating Noise session");
        return NULL;
    }

    memset(n, 0, sizeof(*n));
    n->pattern = pattern;
    n->role = role;
    n->step = 1;
    n->action = (role == CAIS_NOISE_INITIATOR) ? CAIS_NOISE_ACTION_WRITE_MSG : CAIS_NOISE_ACTION_READ_MSG;
    n->is_complete = CAIS_FALSE;

    return n;
}

void cais_noise_free(cais_noise_t *noise)
{
    if (noise != NULL) {
        crypto_wipe(noise, sizeof(*noise));
        free(noise);
    }
}

cais_status_t cais_noise_set_static_key(cais_noise_t *noise,
                                        const unsigned char priv[CAIS_NOISE_KEY_LEN],
                                        char *err_buf, size_t err_cap)
{
    if (noise == NULL || priv == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null argument");
        return CAIS_ERROR;
    }
    memcpy(noise->s_priv, priv, CAIS_NOISE_KEY_LEN);
    crypto_x25519_public_key(noise->s_pub, noise->s_priv);
    noise->has_s = 1;
    return CAIS_OK;
}

cais_status_t cais_noise_set_remote_key(cais_noise_t *noise,
                                        const unsigned char pub[CAIS_NOISE_KEY_LEN],
                                        char *err_buf, size_t err_cap)
{
    if (noise == NULL || pub == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null argument");
        return CAIS_ERROR;
    }
    memcpy(noise->rs, pub, CAIS_NOISE_KEY_LEN);
    noise->has_rs = 1;
    return CAIS_OK;
}

cais_status_t cais_noise_set_ephemeral_key(cais_noise_t *noise,
                                           const unsigned char priv[CAIS_NOISE_KEY_LEN],
                                           char *err_buf, size_t err_cap)
{
    if (noise == NULL || priv == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null argument");
        return CAIS_ERROR;
    }
    memcpy(noise->e_priv, priv, CAIS_NOISE_KEY_LEN);
    crypto_x25519_public_key(noise->e_pub, noise->e_priv);
    noise->has_e = 1;
    return CAIS_OK;
}

cais_status_t cais_noise_get_handshake_hash(const cais_noise_t *noise,
                                            unsigned char out_hash[CAIS_NOISE_HASH_LEN],
                                            char *err_buf, size_t err_cap)
{
    if (noise == NULL || out_hash == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null argument");
        return CAIS_ERROR;
    }
    memcpy(out_hash, noise->sym.h, CAIS_NOISE_HASH_LEN);
    return CAIS_OK;
}

cais_status_t cais_noise_set_prologue(cais_noise_t *noise,
                                      const void *prologue, size_t len,
                                      char *err_buf, size_t err_cap)
{
    if (noise == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null noise session");
        return CAIS_ERROR;
    }
    if (len > sizeof(noise->prologue)) {
        cais_noise_set_err(err_buf, err_cap, "prologue exceeds maximum capacity (256 bytes)");
        return CAIS_ERROR;
    }
    if (len > 0 && prologue != NULL) {
        memcpy(noise->prologue, prologue, len);
    }
    noise->prologue_len = len;
    return CAIS_OK;
}

cais_status_t cais_noise_start(cais_noise_t *noise,
                               char *err_buf, size_t err_cap)
{
    const char *pname;

    if (noise == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null noise session");
        return CAIS_ERROR;
    }

    if (noise->pattern == CAIS_NOISE_XX) {
        if (!noise->has_s) {
            cais_noise_set_err(err_buf, err_cap, "Noise_XX requires a configured static key");
            return CAIS_ERROR;
        }
        pname = "Noise_XX_25519_ChaChaPoly_BLAKE2s";
    } else if (noise->pattern == CAIS_NOISE_NN) {
        pname = "Noise_NN_25519_ChaChaPoly_BLAKE2s";
    } else if (noise->pattern == CAIS_NOISE_NK) {
        if (noise->role == CAIS_NOISE_INITIATOR && !noise->has_rs) {
            cais_noise_set_err(err_buf, err_cap, "Noise_NK initiator requires remote public key");
            return CAIS_ERROR;
        }
        if (noise->role == CAIS_NOISE_RESPONDER && !noise->has_s) {
            cais_noise_set_err(err_buf, err_cap, "Noise_NK responder requires static private key");
            return CAIS_ERROR;
        }
        pname = "Noise_NK_25519_ChaChaPoly_BLAKE2s";
    } else if (noise->pattern == CAIS_NOISE_IK) {
        if (noise->role == CAIS_NOISE_INITIATOR) {
            if (!noise->has_s || !noise->has_rs) {
                cais_noise_set_err(err_buf, err_cap, "Noise_IK initiator requires static key and remote key");
                return CAIS_ERROR;
            }
        } else {
            if (!noise->has_s) {
                cais_noise_set_err(err_buf, err_cap, "Noise_IK responder requires static key");
                return CAIS_ERROR;
            }
        }
        pname = "Noise_IK_25519_ChaChaPoly_BLAKE2s";
    } else {
        cais_noise_set_err(err_buf, err_cap, "unsupported pattern");
        return CAIS_ERROR;
    }

    cais_noise_sym_init(&noise->sym, pname);

    /* Section 5.3: MixHash(prologue) is always executed, even if prologue is empty */
    cais_noise_sym_mix_hash(&noise->sym, noise->prologue, noise->prologue_len);

    /* Pre-messages for NK and IK (<- s) */
    if (noise->pattern == CAIS_NOISE_NK || noise->pattern == CAIS_NOISE_IK) {
        if (noise->role == CAIS_NOISE_INITIATOR) {
            cais_noise_sym_mix_hash(&noise->sym, noise->rs, 32);
        } else {
            cais_noise_sym_mix_hash(&noise->sym, noise->s_pub, 32);
        }
    }
    noise->step = 1;
    noise->action = (noise->role == CAIS_NOISE_INITIATOR) ? CAIS_NOISE_ACTION_WRITE_MSG : CAIS_NOISE_ACTION_READ_MSG;
    noise->is_complete = CAIS_FALSE;

    return CAIS_OK;
}

cais_noise_action_t cais_noise_get_action(const cais_noise_t *noise)
{
    return noise ? noise->action : CAIS_NOISE_ACTION_FAILED;
}

cais_bool_t cais_noise_is_complete(const cais_noise_t *noise)
{
    return (noise && noise->is_complete) ? CAIS_TRUE : CAIS_FALSE;
}

const unsigned char *cais_noise_get_remote_static(const cais_noise_t *noise)
{
    if (noise && noise->has_rs) {
        return noise->rs;
    }
    return NULL;
}

/*
 * =============================================================================
 * Handshake Message Processing: Noise_XX and Noise_NN
 * =============================================================================
 */

cais_status_t cais_noise_write_message(cais_noise_t *noise,
                                       const void *payload, size_t payload_len,
                                       void *out_msg, size_t out_cap, size_t *out_len,
                                       char *err_buf, size_t err_cap)
{
    uint8_t *p = (uint8_t *)out_msg;
    size_t written = 0;
    uint8_t dh[32];

    if (noise == NULL || out_msg == NULL || out_len == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null arguments");
        return CAIS_ERROR;
    }

    if (noise->action != CAIS_NOISE_ACTION_WRITE_MSG) {
        cais_noise_set_err(err_buf, err_cap, "session is not expecting to write a message");
        return CAIS_ERROR;
    }

    /* Pattern: Noise_NN */
    if (noise->pattern == CAIS_NOISE_NN) {
        if (noise->step == 1) {
            /* -> e */
            if (out_cap < 32 + payload_len) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for message 1");
                return CAIS_ERROR;
            }
            if (!noise->has_e) {
                if (cais_noise_keygen(noise->e_priv, noise->e_pub, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
                noise->has_e = 1;
            }
            memcpy(p, noise->e_pub, 32);
            cais_noise_sym_mix_hash(&noise->sym, noise->e_pub, 32);
            p += 32;
            written += 32;

            /* Payload */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;
            noise->step = 2;
            noise->action = CAIS_NOISE_ACTION_READ_MSG;
            return CAIS_OK;
        } else if (noise->step == 2) {
            /* <- e, ee */
            if (out_cap < 32 + payload_len + 16) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for message 2");
                return CAIS_ERROR;
            }
            if (!noise->has_e) {
                if (cais_noise_keygen(noise->e_priv, noise->e_pub, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
                noise->has_e = 1;
            }
            memcpy(p, noise->e_pub, 32);
            cais_noise_sym_mix_hash(&noise->sym, noise->e_pub, 32);
            p += 32;
            written += 32;

            /* ee */
            crypto_x25519(dh, noise->e_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;

            /* Split */
            cais_noise_sym_split(&noise->sym, &noise->rx_cipher, &noise->tx_cipher);
            noise->action = CAIS_NOISE_ACTION_SPLIT;
            noise->is_complete = CAIS_TRUE;
            return CAIS_OK;
        }
    }

    /* Pattern: Noise_XX */
    if (noise->pattern == CAIS_NOISE_XX) {
        if (noise->step == 1) {
            /* Message 1 (-> e): Initiator -> Responder */
            if (out_cap < 32 + payload_len) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for XX msg 1");
                return CAIS_ERROR;
            }
            if (!noise->has_e) {
                if (cais_noise_keygen(noise->e_priv, noise->e_pub, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
                noise->has_e = 1;
            }
            memcpy(p, noise->e_pub, 32);
            cais_noise_sym_mix_hash(&noise->sym, noise->e_pub, 32);
            p += 32;
            written += 32;

            /* EncryptAndHash payload (in the clear, no key yet) */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;
            noise->step = 2;
            noise->action = CAIS_NOISE_ACTION_READ_MSG;
            return CAIS_OK;
        } else if (noise->step == 2) {
            /* Message 2 (<- e, ee, s, es): Responder -> Initiator */
            if (out_cap < 32 + 48 + payload_len + 16) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for XX msg 2");
                return CAIS_ERROR;
            }
            if (!noise->has_e) {
                if (cais_noise_keygen(noise->e_priv, noise->e_pub, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
                noise->has_e = 1;
            }
            memcpy(p, noise->e_pub, 32);
            cais_noise_sym_mix_hash(&noise->sym, noise->e_pub, 32);
            p += 32;
            written += 32;

            /* ee */
            crypto_x25519(dh, noise->e_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);

            /* s (encrypted) */
            {
                size_t slen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, noise->s_pub, 32, p, &slen);
                p += slen;
                written += slen;
            }

            /* es */
            crypto_x25519(dh, noise->s_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;
            noise->step = 3;
            noise->action = CAIS_NOISE_ACTION_READ_MSG;
            return CAIS_OK;
        } else if (noise->step == 3) {
            /* Message 3 (-> s, se): Initiator -> Responder */
            if (out_cap < 48 + payload_len + 16) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for XX msg 3");
                return CAIS_ERROR;
            }

            /* s (encrypted) */
            {
                size_t slen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, noise->s_pub, 32, p, &slen);
                p += slen;
                written += slen;
            }

            /* se */
            crypto_x25519(dh, noise->s_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;

            /* Split (Initiator) */
            cais_noise_sym_split(&noise->sym, &noise->tx_cipher, &noise->rx_cipher);
            noise->action = CAIS_NOISE_ACTION_SPLIT;
            noise->is_complete = CAIS_TRUE;
            return CAIS_OK;
        }
    }

    /* Pattern: Noise_NK */
    if (noise->pattern == CAIS_NOISE_NK) {
        if (noise->step == 1) {
            /* -> e, es */
            if (out_cap < 32 + payload_len + 16) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for NK msg 1");
                return CAIS_ERROR;
            }
            if (!noise->has_e) {
                if (cais_noise_keygen(noise->e_priv, noise->e_pub, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
                noise->has_e = 1;
            }
            memcpy(p, noise->e_pub, 32);
            cais_noise_sym_mix_hash(&noise->sym, noise->e_pub, 32);
            p += 32;
            written += 32;

            /* es: DH(e, rs) */
            crypto_x25519(dh, noise->e_priv, noise->rs);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;
            noise->step = 2;
            noise->action = CAIS_NOISE_ACTION_READ_MSG;
            return CAIS_OK;
        } else if (noise->step == 2) {
            /* <- e, ee */
            if (out_cap < 32 + payload_len + 16) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for NK msg 2");
                return CAIS_ERROR;
            }
            if (!noise->has_e) {
                if (cais_noise_keygen(noise->e_priv, noise->e_pub, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
                noise->has_e = 1;
            }
            memcpy(p, noise->e_pub, 32);
            cais_noise_sym_mix_hash(&noise->sym, noise->e_pub, 32);
            p += 32;
            written += 32;

            /* ee: DH(e, re) */
            crypto_x25519(dh, noise->e_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;

            /* Split (Responder: rx=c1, tx=c2) */
            cais_noise_sym_split(&noise->sym, &noise->rx_cipher, &noise->tx_cipher);
            noise->action = CAIS_NOISE_ACTION_SPLIT;
            noise->is_complete = CAIS_TRUE;
            return CAIS_OK;
        }
    }

    /* Pattern: Noise_IK */
    if (noise->pattern == CAIS_NOISE_IK) {
        if (noise->step == 1) {
            /* -> e, es, s, ss */
            if (out_cap < 32 + 48 + payload_len + 16) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for IK msg 1");
                return CAIS_ERROR;
            }
            if (!noise->has_e) {
                if (cais_noise_keygen(noise->e_priv, noise->e_pub, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
                noise->has_e = 1;
            }
            memcpy(p, noise->e_pub, 32);
            cais_noise_sym_mix_hash(&noise->sym, noise->e_pub, 32);
            p += 32;
            written += 32;

            /* es: DH(e, rs) */
            crypto_x25519(dh, noise->e_priv, noise->rs);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);

            /* s (encrypted) */
            {
                size_t slen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, noise->s_pub, 32, p, &slen);
                p += slen;
                written += slen;
            }

            /* ss: DH(s, rs) */
            crypto_x25519(dh, noise->s_priv, noise->rs);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;
            noise->step = 2;
            noise->action = CAIS_NOISE_ACTION_READ_MSG;
            return CAIS_OK;
        } else if (noise->step == 2) {
            /* <- e, ee, se */
            if (out_cap < 32 + payload_len + 16) {
                cais_noise_set_err(err_buf, err_cap, "output buffer too small for IK msg 2");
                return CAIS_ERROR;
            }
            if (!noise->has_e) {
                if (cais_noise_keygen(noise->e_priv, noise->e_pub, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
                noise->has_e = 1;
            }
            memcpy(p, noise->e_pub, 32);
            cais_noise_sym_mix_hash(&noise->sym, noise->e_pub, 32);
            p += 32;
            written += 32;

            /* ee: DH(e, re) */
            crypto_x25519(dh, noise->e_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);

            /* se: DH(e, rs) - responder e, initiator static rs */
            crypto_x25519(dh, noise->e_priv, noise->rs);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            {
                size_t plen = 0;
                cais_noise_sym_encrypt_and_hash(&noise->sym, payload, payload_len, p, &plen);
                written += plen;
            }

            *out_len = written;

            /* Split (Responder: rx=c1, tx=c2) */
            cais_noise_sym_split(&noise->sym, &noise->rx_cipher, &noise->tx_cipher);
            noise->action = CAIS_NOISE_ACTION_SPLIT;
            noise->is_complete = CAIS_TRUE;
            return CAIS_OK;
        }
    }

    cais_noise_set_err(err_buf, err_cap, "unexpected handshake sequence state");
    return CAIS_ERROR;
}

cais_status_t cais_noise_read_message(cais_noise_t *noise,
                                      const void *in_msg, size_t in_len,
                                      void *out_payload, size_t out_cap, size_t *out_payload_len,
                                      char *err_buf, size_t err_cap)
{
    const uint8_t *p = (const uint8_t *)in_msg;
    size_t remaining = in_len;
    uint8_t dh[32];
    (void)out_cap;

    if (noise == NULL || in_msg == NULL || out_payload_len == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null arguments");
        return CAIS_ERROR;
    }

    if (noise->action != CAIS_NOISE_ACTION_READ_MSG) {
        cais_noise_set_err(err_buf, err_cap, "session is not expecting to read a message");
        return CAIS_ERROR;
    }

    /* Pattern: Noise_NN */
    if (noise->pattern == CAIS_NOISE_NN) {
        if (noise->step == 1) {
            /* Read -> e */
            if (remaining < 32) {
                cais_noise_set_err(err_buf, err_cap, "message 1 too short for re");
                return CAIS_ERROR;
            }
            memcpy(noise->re, p, 32);
            noise->has_re = 1;
            cais_noise_sym_mix_hash(&noise->sym, noise->re, 32);
            p += 32;
            remaining -= 32;

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "failed decrypting payload in message 1");
                return CAIS_ERROR;
            }

            noise->step = 2;
            noise->action = CAIS_NOISE_ACTION_WRITE_MSG;
            return CAIS_OK;
        } else if (noise->step == 2) {
            /* Read <- e, ee */
            if (remaining < 32 + 16) {
                cais_noise_set_err(err_buf, err_cap, "message 2 too short");
                return CAIS_ERROR;
            }
            memcpy(noise->re, p, 32);
            noise->has_re = 1;
            cais_noise_sym_mix_hash(&noise->sym, noise->re, 32);
            p += 32;
            remaining -= 32;

            /* ee */
            crypto_x25519(dh, noise->e_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "authentication failed on message 2 payload");
                return CAIS_ERROR;
            }

            /* Split (Initiator) */
            cais_noise_sym_split(&noise->sym, &noise->tx_cipher, &noise->rx_cipher);
            noise->action = CAIS_NOISE_ACTION_SPLIT;
            noise->is_complete = CAIS_TRUE;
            return CAIS_OK;
        }
    }

    /* Pattern: Noise_XX */
    if (noise->pattern == CAIS_NOISE_XX) {
        if (noise->step == 1) {
            /* Read -> e */
            if (remaining < 32) {
                cais_noise_set_err(err_buf, err_cap, "message 1 too short for re");
                return CAIS_ERROR;
            }
            memcpy(noise->re, p, 32);
            noise->has_re = 1;
            cais_noise_sym_mix_hash(&noise->sym, noise->re, 32);
            p += 32;
            remaining -= 32;

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "failed decrypting payload in message 1");
                return CAIS_ERROR;
            }

            noise->step = 2;
            noise->action = CAIS_NOISE_ACTION_WRITE_MSG;
            return CAIS_OK;
        } else if (noise->step == 2) {
            /* Read <- e, ee, s, es */
            if (remaining < 32 + 48 + 16) {
                cais_noise_set_err(err_buf, err_cap, "message 2 too short for XX");
                return CAIS_ERROR;
            }
            memcpy(noise->re, p, 32);
            noise->has_re = 1;
            cais_noise_sym_mix_hash(&noise->sym, noise->re, 32);
            p += 32;
            remaining -= 32;

            /* ee */
            crypto_x25519(dh, noise->e_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);

            /* s (encrypted) */
            {
                size_t slen = 0;
                if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, 48, noise->rs, &slen) != 0 || slen != 32) {
                    cais_noise_set_err(err_buf, err_cap, "failed decrypting remote static key in XX msg 2");
                    return CAIS_ERROR;
                }
                noise->has_rs = 1;
                p += 48;
                remaining -= 48;
            }

            /* es */
            crypto_x25519(dh, noise->e_priv, noise->rs);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "authentication failed on XX message 2 payload");
                return CAIS_ERROR;
            }

            noise->step = 3;
            noise->action = CAIS_NOISE_ACTION_WRITE_MSG;
            return CAIS_OK;
        } else if (noise->step == 3) {
            /* Read -> s, se */
            if (remaining < 48 + 16) {
                cais_noise_set_err(err_buf, err_cap, "message 3 too short for XX");
                return CAIS_ERROR;
            }

            /* s (encrypted) */
            {
                size_t slen = 0;
                if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, 48, noise->rs, &slen) != 0 || slen != 32) {
                    cais_noise_set_err(err_buf, err_cap, "failed decrypting remote static key in XX msg 3");
                    return CAIS_ERROR;
                }
                noise->has_rs = 1;
                p += 48;
                remaining -= 48;
            }

            /* se */
            crypto_x25519(dh, noise->e_priv, noise->rs);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "authentication failed on XX message 3 payload");
                return CAIS_ERROR;
            }

            /* Split (Responder) */
            cais_noise_sym_split(&noise->sym, &noise->rx_cipher, &noise->tx_cipher);
            noise->action = CAIS_NOISE_ACTION_SPLIT;
            noise->is_complete = CAIS_TRUE;
            return CAIS_OK;
        }
    }

    /* Pattern: Noise_NK */
    if (noise->pattern == CAIS_NOISE_NK) {
        if (noise->step == 1) {
            /* Read -> e, es */
            if (remaining < 32 + 16) {
                cais_noise_set_err(err_buf, err_cap, "NK msg 1 too short");
                return CAIS_ERROR;
            }
            memcpy(noise->re, p, 32);
            noise->has_re = 1;
            cais_noise_sym_mix_hash(&noise->sym, noise->re, 32);
            p += 32;
            remaining -= 32;

            /* es: DH(s, re) */
            crypto_x25519(dh, noise->s_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "authentication failed on NK msg 1 payload");
                return CAIS_ERROR;
            }

            noise->step = 2;
            noise->action = CAIS_NOISE_ACTION_WRITE_MSG;
            return CAIS_OK;
        } else if (noise->step == 2) {
            /* Read <- e, ee */
            if (remaining < 32 + 16) {
                cais_noise_set_err(err_buf, err_cap, "NK msg 2 too short");
                return CAIS_ERROR;
            }
            memcpy(noise->re, p, 32);
            noise->has_re = 1;
            cais_noise_sym_mix_hash(&noise->sym, noise->re, 32);
            p += 32;
            remaining -= 32;

            /* ee: DH(e, re) */
            crypto_x25519(dh, noise->e_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "authentication failed on NK msg 2 payload");
                return CAIS_ERROR;
            }

            /* Split (Initiator: tx=c1, rx=c2) */
            cais_noise_sym_split(&noise->sym, &noise->tx_cipher, &noise->rx_cipher);
            noise->action = CAIS_NOISE_ACTION_SPLIT;
            noise->is_complete = CAIS_TRUE;
            return CAIS_OK;
        }
    }

    /* Pattern: Noise_IK */
    if (noise->pattern == CAIS_NOISE_IK) {
        if (noise->step == 1) {
            /* Read -> e, es, s, ss */
            if (remaining < 32 + 48 + 16) {
                cais_noise_set_err(err_buf, err_cap, "IK msg 1 too short");
                return CAIS_ERROR;
            }
            memcpy(noise->re, p, 32);
            noise->has_re = 1;
            cais_noise_sym_mix_hash(&noise->sym, noise->re, 32);
            p += 32;
            remaining -= 32;

            /* es: DH(s, re) */
            crypto_x25519(dh, noise->s_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);

            /* s (encrypted) */
            {
                size_t slen = 0;
                if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, 48, noise->rs, &slen) != 0 || slen != 32) {
                    cais_noise_set_err(err_buf, err_cap, "failed decrypting remote static key in IK msg 1");
                    return CAIS_ERROR;
                }
                noise->has_rs = 1;
                p += 48;
                remaining -= 48;
            }

            /* ss: DH(s, rs) */
            crypto_x25519(dh, noise->s_priv, noise->rs);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "authentication failed on IK msg 1 payload");
                return CAIS_ERROR;
            }

            noise->step = 2;
            noise->action = CAIS_NOISE_ACTION_WRITE_MSG;
            return CAIS_OK;
        } else if (noise->step == 2) {
            /* Read <- e, ee, se */
            if (remaining < 32 + 16) {
                cais_noise_set_err(err_buf, err_cap, "IK msg 2 too short");
                return CAIS_ERROR;
            }
            memcpy(noise->re, p, 32);
            noise->has_re = 1;
            cais_noise_sym_mix_hash(&noise->sym, noise->re, 32);
            p += 32;
            remaining -= 32;

            /* ee: DH(e, re) */
            crypto_x25519(dh, noise->e_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);

            /* se: DH(s, re) - initiator static s, responder ephemeral re */
            crypto_x25519(dh, noise->s_priv, noise->re);
            cais_noise_sym_mix_key(&noise->sym, dh, 32);
            crypto_wipe(dh, sizeof(dh));

            /* Payload */
            if (cais_noise_sym_decrypt_and_hash(&noise->sym, p, remaining, out_payload, out_payload_len) != 0) {
                cais_noise_set_err(err_buf, err_cap, "authentication failed on IK msg 2 payload");
                return CAIS_ERROR;
            }

            /* Split (Initiator: tx=c1, rx=c2) */
            cais_noise_sym_split(&noise->sym, &noise->tx_cipher, &noise->rx_cipher);
            noise->action = CAIS_NOISE_ACTION_SPLIT;
            noise->is_complete = CAIS_TRUE;
            return CAIS_OK;
        }
    }

    cais_noise_set_err(err_buf, err_cap, "unexpected handshake read state");
    return CAIS_ERROR;
}

/*
 * =============================================================================
 * Transport Phase Encryption / Decryption
 * =============================================================================
 */

cais_status_t cais_noise_encrypt(cais_noise_t *noise,
                                 const void *plain, size_t plain_len,
                                 void *out_cipher, size_t out_cap, size_t *out_len,
                                 char *err_buf, size_t err_cap)
{
    uint8_t mac[16];

    if (noise == NULL || out_cipher == NULL || out_len == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null argument");
        return CAIS_ERROR;
    }

    if (!noise->is_complete || !noise->tx_cipher.has_key) {
        cais_noise_set_err(err_buf, err_cap, "noise session is not in transport state");
        return CAIS_ERROR;
    }

    if (out_cap < plain_len + 16) {
        cais_noise_set_err(err_buf, err_cap, "output buffer too small for ciphertext + MAC");
        return CAIS_ERROR;
    }

    if (plain_len > 0 && plain != NULL) {
        cais_noise_aead_encrypt((uint8_t *)out_cipher, mac,
                                noise->tx_cipher.k, noise->tx_cipher.n,
                                NULL, 0,
                                (const uint8_t *)plain, plain_len);
    } else {
        cais_noise_aead_encrypt((uint8_t *)out_cipher, mac,
                                noise->tx_cipher.k, noise->tx_cipher.n,
                                NULL, 0,
                                NULL, 0);
    }

    memcpy((uint8_t *)out_cipher + plain_len, mac, 16);
    noise->tx_cipher.n++;
    *out_len = plain_len + 16;
    return CAIS_OK;
}

cais_status_t cais_noise_decrypt(cais_noise_t *noise,
                                 const void *cipher, size_t cipher_len,
                                 void *out_plain, size_t out_cap, size_t *out_len,
                                 char *err_buf, size_t err_cap)
{
    uint8_t mac[16];
    size_t plainlen;

    if (noise == NULL || cipher == NULL || out_plain == NULL || out_len == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null argument");
        return CAIS_ERROR;
    }

    if (!noise->is_complete || !noise->rx_cipher.has_key) {
        cais_noise_set_err(err_buf, err_cap, "noise session is not in transport state");
        return CAIS_ERROR;
    }

    if (cipher_len < 16) {
        cais_noise_set_err(err_buf, err_cap, "ciphertext too short (must be at least 16 bytes for MAC)");
        return CAIS_ERROR;
    }

    plainlen = cipher_len - 16;
    if (out_cap < plainlen) {
        cais_noise_set_err(err_buf, err_cap, "output buffer too small for plaintext");
        return CAIS_ERROR;
    }

    memcpy(mac, (const uint8_t *)cipher + plainlen, 16);

    if (cais_noise_aead_decrypt((uint8_t *)out_plain,
                                noise->rx_cipher.k, noise->rx_cipher.n,
                                mac,
                                NULL, 0,
                                (const uint8_t *)cipher, plainlen) != 0) {
        cais_noise_set_err(err_buf, err_cap, "transport message authentication failed (MAC mismatch)");
        return CAIS_ERROR;
    }

    noise->rx_cipher.n++;
    *out_len = plainlen;
    return CAIS_OK;
}
