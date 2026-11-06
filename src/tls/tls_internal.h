/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_TLS_INTERNAL_H
#define CAIS_TLS_INTERNAL_H

#include <cais/tls.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Backend Driver VTable Interface
 */
typedef struct cais_tls_backend_ops {
    cais_tls_backend_type_t type;
    const char             *name;

    int  (*init)(cais_tls_t *tls, const cais_tls_config_t *conf, char *err_buf, size_t err_cap);
    int  (*handshake)(cais_tls_t *tls, char *err_buf, size_t err_cap);
    int  (*send)(cais_tls_t *tls, const void *data, size_t len, size_t *written_bytes, char *err_buf, size_t err_cap);
    int  (*recv)(cais_tls_t *tls, void *buffer, size_t len, size_t *read_bytes, char *err_buf, size_t err_cap);
    int  (*close)(cais_tls_t *tls);
    void (*free)(cais_tls_t *tls);
} cais_tls_backend_ops_t;

/* Returns the backend ops table configured for the active compile target */
const cais_tls_backend_ops_t *cais_tls_get_backend_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_TLS_INTERNAL_H */
