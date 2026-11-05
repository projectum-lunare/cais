/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_SERIAL_H
#define CAIS_SERIAL_H

#include <stddef.h>
#include <cais/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct serial_port cais_serial_t;

/*
 * Serial flow control modes.
 */
typedef enum {
    CAIS_SERIAL_FLOW_NONE     = 0,
    CAIS_SERIAL_FLOW_HARDWARE = 1, /* RTS/CTS */
    CAIS_SERIAL_FLOW_SOFTWARE = 2  /* XON/XOFF */
} cais_serial_flow_t;

/*
 * Serial parity modes.
 */
typedef enum {
    CAIS_SERIAL_PARITY_NONE  = 'n',
    CAIS_SERIAL_PARITY_EVEN  = 'e',
    CAIS_SERIAL_PARITY_ODD   = 'o',
    CAIS_SERIAL_PARITY_MARK  = 'm',
    CAIS_SERIAL_PARITY_SPACE = 's'
} cais_serial_parity_t;

/*
 * Serial stop bit counts.
 */
typedef enum {
    CAIS_SERIAL_STOP_1 = 1,
    CAIS_SERIAL_STOP_2 = 2
} cais_serial_stop_bits_t;

/*
 * Serial data bit lengths.
 */
typedef enum {
    CAIS_SERIAL_DATA_5 = 5,
    CAIS_SERIAL_DATA_6 = 6,
    CAIS_SERIAL_DATA_7 = 7,
    CAIS_SERIAL_DATA_8 = 8
} cais_serial_data_bits_t;

/*
 * Standard serial baud rates.
 */
typedef enum {
    CAIS_SERIAL_BAUD_300    = 300,
    CAIS_SERIAL_BAUD_1200   = 1200,
    CAIS_SERIAL_BAUD_2400   = 2400,
    CAIS_SERIAL_BAUD_4800   = 4800,
    CAIS_SERIAL_BAUD_9600   = 9600,
    CAIS_SERIAL_BAUD_19200  = 19200,
    CAIS_SERIAL_BAUD_38400  = 38400,
    CAIS_SERIAL_BAUD_57600  = 57600,
    CAIS_SERIAL_BAUD_115200 = 115200
} cais_serial_baud_t;

/*
 * Serial modem line states (RTS/DTR).
 */
typedef enum {
    CAIS_SERIAL_LINE_IGNORE = -1,
    CAIS_SERIAL_LINE_OFF    =  0,
    CAIS_SERIAL_LINE_ON     =  1
} cais_serial_line_t;

/* Backward-compatible hardware signal line aliases */
typedef cais_line_state_t cais_serial_line_state_t;
#define CAIS_SERIAL_LINE_ERROR CAIS_LINE_ERROR
#define CAIS_SERIAL_LINE_LOW   CAIS_LINE_LOW
#define CAIS_SERIAL_LINE_HIGH  CAIS_LINE_HIGH

/**
 * @brief Configuration options for opening a serial UART port.
 */
typedef struct cais_serial_opts {
    int  baud;         /**< Baud rate (e.g. 9600, 115200, or cais_serial_baud_t) */
    int  data_bits;    /**< Data bits length: 5, 6, 7, or 8 (cais_serial_data_bits_t) */
    char parity;       /**< Parity scheme: 'n' (none), 'e' (even), 'o' (odd), 'm' (mark), 's' (space) */
    int  stop_bits;    /**< Stop bits count: 1 or 2 (cais_serial_stop_bits_t) */
    int  flow_control; /**< Flow control mode: 0 (none), 1 (RTS/CTS hardware), 2 (XON/XOFF software) */
    int  rts;          /**< Initial RTS line state: -1 (ignore), 0 (low/clear), 1 (high/set) */
    int  dtr;          /**< Initial DTR line state: -1 (ignore), 0 (low/clear), 1 (high/set) */
    int  timeout_ms;   /**< I/O timeout: <0 (blocking indefinitely), 0 (non-blocking), >0 (timeout in ms) */
} cais_serial_opts_t;

/*
 * Opens a serial port for communication.
 * Default flow control is CAIS_SERIAL_FLOW_NONE (0).
 * Default timeout is -1 (blocking indefinitely).
 * Hardware flow control (RTS/CTS) is only activated when explicitly requested.
 * On failure, returns NULL and writes descriptive error text into err_buf (if non-NULL).
 * Does not allocate dynamic heap memory for error strings.
 */
cais_serial_t *cais_serial_open(const char *path, const cais_serial_opts_t *opts, char *err_buf, size_t err_cap);

/*
 * Writes data to the serial port.
 * Returns:
 *   CAIS_OK          : all requested bytes written, or partial bytes written before timeout.
 *   CAIS_AGAIN       : timeout expired before any bytes could be accepted.
 *   CAIS_ERROR       : fatal I/O or descriptor error.
 * If written_bytes != NULL, stores the count of bytes successfully accepted.
 */
cais_status_t cais_serial_write(cais_serial_t *port, const char *buf, size_t len, size_t *written_bytes);

/*
 * Reads data from the serial port.
 * Returns:
 *   CAIS_OK          : one or more bytes read into buf (*read_bytes populated).
 *   CAIS_AGAIN       : timeout expired before any bytes became available.
 *   CAIS_ERROR       : fatal I/O error or descriptor invalid.
 *   CAIS_UNSUPPORTED : timed read not supported on platform.
 *
 * If timeout_ms < 0, uses the port's configured timeout (from opts or cais_serial_set_timeout).
 * If the effective timeout is < 0, blocks indefinitely until at least one byte is available.
 * If the effective timeout is 0, performs a non-blocking check.
 * If the effective timeout is > 0, waits up to timeout_ms milliseconds.
 */
cais_status_t cais_serial_read(cais_serial_t *port, char *buf, size_t max_len, size_t *read_bytes, int timeout_ms);

/*
 * Configures the default I/O timeout in milliseconds for the serial port.
 */
cais_serial_status_t cais_serial_set_timeout(cais_serial_t *port, int timeout_ms);

/*
 * Waits for the UART transmit buffer to drain completely (hardware drain).
 * Returns CAIS_SERIAL_OK on success, or CAIS_SERIAL_ERROR on failure.
 */
cais_serial_status_t cais_serial_drain(cais_serial_t *port);

/*
 * Controls the RTS and DTR modem lines of the UART.
 * Returns CAIS_SERIAL_OK on success, CAIS_SERIAL_ERROR on I/O error,
 * or CAIS_SERIAL_UNSUPPORTED if modem line control is not supported.
 */
cais_serial_status_t cais_serial_set_rts(cais_serial_t *port, cais_serial_line_t state);
cais_serial_status_t cais_serial_set_dtr(cais_serial_t *port, cais_serial_line_t state);

/*
 * Queries the modem input lines (CTS, DSR, CD/DCD, RI).
 * Returns CAIS_LINE_HIGH if asserted/high, CAIS_LINE_LOW if deasserted/low,
 * or CAIS_LINE_ERROR on error or if unsupported.
 */
cais_line_state_t cais_serial_get_cts(cais_serial_t *port);
cais_line_state_t cais_serial_get_dsr(cais_serial_t *port);
cais_line_state_t cais_serial_get_cd(cais_serial_t *port);
cais_line_state_t cais_serial_get_ri(cais_serial_t *port);

/*
 * Closes the serial port and frees allocated resources.
 */
void cais_serial_close(cais_serial_t *port);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_SERIAL_H */
