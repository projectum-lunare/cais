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
#include <cais/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Helper to reliably write exact number of bytes over stream socket.
 */
static cais_status_t stream_write_all(cais_socket_t *sock, const uint8_t *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        size_t sent = 0;
        cais_socket_status_t st = cais_socket_send(sock, (const char *)buf + total, len - total, &sent);
        if (st == CAIS_SOCK_OK) {
            total += sent;
        } else if (st == CAIS_SOCK_AGAIN) {
            continue;
        } else {
            return CAIS_ERROR;
        }
    }
    return CAIS_OK;
}

/*
 * Helper to reliably read exact number of bytes over stream socket.
 */
static cais_status_t stream_read_all(cais_socket_t *sock, uint8_t *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        size_t recvd = 0;
        cais_socket_status_t st = cais_socket_recv(sock, (char *)buf + total, len - total, &recvd);
        if (st == CAIS_SOCK_OK) {
            if (recvd == 0) {
                return CAIS_EOF;
            }
            total += recvd;
        } else if (st == CAIS_SOCK_AGAIN) {
            continue;
        } else if (st == CAIS_SOCK_CLOSED) {
            return CAIS_EOF;
        } else {
            return CAIS_ERROR;
        }
    }
    return CAIS_OK;
}

cais_status_t cais_noise_handshake_socket(cais_noise_t *noise,
                                          struct cais_socket *sock,
                                          char *err_buf, size_t err_cap)
{
    uint8_t msg_buf[256];
    uint8_t payload_buf[64];
    size_t msg_len = 0;
    size_t payload_len = 0;

    if (noise == NULL || sock == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null arguments to handshake_socket");
        return CAIS_ERROR;
    }

    while (!cais_noise_is_complete(noise)) {
        cais_noise_action_t action = cais_noise_get_action(noise);

        if (action == CAIS_NOISE_ACTION_WRITE_MSG) {
            uint8_t hdr[2];
            if (cais_noise_write_message(noise, NULL, 0, msg_buf, sizeof(msg_buf), &msg_len, err_buf, err_cap) != CAIS_OK) {
                return CAIS_ERROR;
            }
            hdr[0] = (uint8_t)((msg_len >> 8) & 0xFF);
            hdr[1] = (uint8_t)(msg_len & 0xFF);
            if (stream_write_all(sock, hdr, 2) != CAIS_OK ||
                stream_write_all(sock, msg_buf, msg_len) != CAIS_OK) {
                cais_noise_set_err(err_buf, err_cap, "I/O error sending handshake frame");
                return CAIS_ERROR;
            }
        } else if (action == CAIS_NOISE_ACTION_READ_MSG) {
            uint8_t hdr[2];
            size_t frame_len;
            cais_status_t st = stream_read_all(sock, hdr, 2);
            if (st != CAIS_OK) {
                cais_noise_set_err(err_buf, err_cap, "peer closed or I/O error reading handshake header");
                return st;
            }
            frame_len = (((size_t)hdr[0]) << 8) | ((size_t)hdr[1]);
            if (frame_len > sizeof(msg_buf)) {
                cais_noise_set_err(err_buf, err_cap, "incoming handshake frame exceeds buffer capacity");
                return CAIS_ERROR;
            }
            if (stream_read_all(sock, msg_buf, frame_len) != CAIS_OK) {
                cais_noise_set_err(err_buf, err_cap, "I/O error reading handshake frame body");
                return CAIS_ERROR;
            }
            if (cais_noise_read_message(noise, msg_buf, frame_len, payload_buf, sizeof(payload_buf), &payload_len, err_buf, err_cap) != CAIS_OK) {
                return CAIS_ERROR;
            }
        } else {
            break;
        }
    }

    return cais_noise_is_complete(noise) ? CAIS_OK : CAIS_ERROR;
}

cais_status_t cais_noise_send_socket(cais_noise_t *noise,
                                     struct cais_socket *sock,
                                     const void *data, size_t len, size_t *sent,
                                     char *err_buf, size_t err_cap)
{
    uint8_t cipher_buf[CAIS_NOISE_MAX_MSG_LEN + 16];
    uint8_t hdr[2];
    size_t cipher_len = 0;

    if (noise == NULL || sock == NULL || (len > 0 && data == NULL)) {
        cais_noise_set_err(err_buf, err_cap, "invalid null arguments");
        return CAIS_ERROR;
    }

    if (len > CAIS_NOISE_MAX_MSG_LEN - 16) {
        cais_noise_set_err(err_buf, err_cap, "plaintext exceeds Noise maximum message length (65519 bytes)");
        return CAIS_ERROR;
    }

    if (cais_noise_encrypt(noise, data, len, cipher_buf, sizeof(cipher_buf), &cipher_len, err_buf, err_cap) != CAIS_OK) {
        return CAIS_ERROR;
    }

    hdr[0] = (uint8_t)((cipher_len >> 8) & 0xFF);
    hdr[1] = (uint8_t)(cipher_len & 0xFF);

    if (stream_write_all(sock, hdr, 2) != CAIS_OK ||
        stream_write_all(sock, cipher_buf, cipher_len) != CAIS_OK) {
        cais_noise_set_err(err_buf, err_cap, "socket write failed while sending Noise transport frame");
        return CAIS_ERROR;
    }

    if (sent != NULL) {
        *sent = len;
    }
    return CAIS_OK;
}

cais_status_t cais_noise_recv_socket(cais_noise_t *noise,
                                     struct cais_socket *sock,
                                     void *buf, size_t cap, size_t *received,
                                     char *err_buf, size_t err_cap)
{
    uint8_t hdr[2];
    uint8_t cipher_buf[CAIS_NOISE_MAX_MSG_LEN + 16];
    size_t cipher_len;
    cais_status_t st;

    if (noise == NULL || sock == NULL || buf == NULL || received == NULL) {
        cais_noise_set_err(err_buf, err_cap, "invalid null arguments");
        return CAIS_ERROR;
    }

    st = stream_read_all(sock, hdr, 2);
    if (st != CAIS_OK) {
        return st;
    }

    cipher_len = (((size_t)hdr[0]) << 8) | ((size_t)hdr[1]);
    if (cipher_len > sizeof(cipher_buf)) {
        cais_noise_set_err(err_buf, err_cap, "incoming transport frame exceeds maximum allowable capacity");
        return CAIS_ERROR;
    }

    st = stream_read_all(sock, cipher_buf, cipher_len);
    if (st != CAIS_OK) {
        cais_noise_set_err(err_buf, err_cap, "socket read failed while receiving Noise frame payload");
        return st;
    }

    return cais_noise_decrypt(noise, cipher_buf, cipher_len, buf, cap, received, err_buf, err_cap);
}
