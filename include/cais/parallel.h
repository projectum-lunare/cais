/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_PARALLEL_H
#define CAIS_PARALLEL_H

#include <stddef.h>
#include <cais/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct parallel_port cais_parallel_t;

/*
 * IEEE 1284 / Centronics parallel port modes.
 */
typedef enum {
    CAIS_PARALLEL_MODE_SPP    = 0, /* Standard Parallel Port (Centronics compatibility) */
    CAIS_PARALLEL_MODE_NIBBLE = 1, /* IEEE 1284 4-bit reverse read */
    CAIS_PARALLEL_MODE_BYTE   = 2, /* IEEE 1284 8-bit bidirectional (PS/2) */
    CAIS_PARALLEL_MODE_EPP    = 3, /* Enhanced Parallel Port */
    CAIS_PARALLEL_MODE_ECP    = 4  /* Extended Capabilities Port (FIFO hardware) */
} cais_parallel_mode_t;

/*
 * Centronics status pins & handshake line flags.
 */
typedef enum {
    CAIS_PARALLEL_PIN_BUSY  = (1 << 0), /* Printer busy */
    CAIS_PARALLEL_PIN_ACK   = (1 << 1), /* Acknowledge pulse */
    CAIS_PARALLEL_PIN_PE    = (1 << 2), /* Paper End / Out */
    CAIS_PARALLEL_PIN_SLCT  = (1 << 3), /* Select / Online */
    CAIS_PARALLEL_PIN_FAULT = (1 << 4)  /* Error / Fault */
} cais_parallel_pin_t;

/*
 * Opens a parallel / Centronics port (IEEE 1284 / LPT) for direct
 * communication with impact printers.
 * On failure, returns NULL and writes descriptive error text into err_buf (if non-NULL).
 * Does not allocate dynamic heap memory for error strings.
 */
cais_parallel_t *cais_parallel_open(const char *path, char *err_buf, size_t err_cap);

/*
 * Writes data to the parallel port. Returns CAIS_OK on success (with *written_bytes populated),
 * or CAIS_ERROR on failure.
 */
cais_status_t cais_parallel_write(cais_parallel_t *port, const char *buf, size_t len, size_t *written_bytes);

/*
 * Reads reverse data from the parallel port (IEEE 1284 Nibble/Byte/EPP/ECP modes).
 * Returns CAIS_OK on success (with *read_bytes populated), CAIS_UNSUPPORTED if mode/hardware
 * does not support reverse reading, or CAIS_ERROR on failure.
 */
cais_status_t cais_parallel_read(cais_parallel_t *port, char *buf, size_t max_len, size_t *read_bytes);

/*
 * Queries current Centronics status pins (BUSY, ACK, PE, SLCT, FAULT).
 * Returns CAIS_PARALLEL_OK and populates *pins_out with cais_parallel_pin_t bitmask,
 * or CAIS_PARALLEL_UNSUPPORTED / CAIS_PARALLEL_ERROR on failure.
 */
cais_parallel_status_t cais_parallel_get_status(cais_parallel_t *port, int *pins_out);

/*
 * Configures the operational mode (SPP, NIBBLE, BYTE, EPP, ECP).
 * Returns CAIS_PARALLEL_OK, or CAIS_PARALLEL_UNSUPPORTED if the mode
 * is not supported by the hardware or driver.
 */
cais_parallel_status_t cais_parallel_set_mode(cais_parallel_t *port, cais_parallel_mode_t mode);

/*
 * Waits for the printer to empty its hardware buffer and deassert BUSY (hardware drain).
 * Returns CAIS_PARALLEL_OK on success, or CAIS_PARALLEL_ERROR on timeout or failure.
 */
cais_parallel_status_t cais_parallel_drain(cais_parallel_t *port);

/*
 * Closes the parallel port and frees allocated resources.
 */
void cais_parallel_close(cais_parallel_t *port);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_PARALLEL_H */
