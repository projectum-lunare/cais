/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cais/platform.h>

#if CAIS_IS_WINDOWS_NT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cais/serial.h>

#include <windows.h>

struct serial_port {
    HANDLE handle;
    DCB orig_dcb;
    int timeout_ms;
};

static cais_serial_status_t cais_serial_apply_comm_timeouts(HANDLE handle, int timeout_ms)
{
    COMMTIMEOUTS to;
    memset(&to, 0, sizeof(to));
    if (timeout_ms > 0) {
        /* Timed: wait up to timeout_ms for first byte, return immediately if already buffered */
        to.ReadIntervalTimeout = MAXDWORD;
        to.ReadTotalTimeoutMultiplier = MAXDWORD;
        to.ReadTotalTimeoutConstant = (DWORD)timeout_ms;
        to.WriteTotalTimeoutMultiplier = 0;
        to.WriteTotalTimeoutConstant = (DWORD)timeout_ms;
    } else if (timeout_ms == 0) {
        /* Non-blocking: return immediately with whatever is in buffer (0 if empty) */
        to.ReadIntervalTimeout = MAXDWORD;
        to.ReadTotalTimeoutMultiplier = 0;
        to.ReadTotalTimeoutConstant = 0;
        to.WriteTotalTimeoutMultiplier = 0;
        to.WriteTotalTimeoutConstant = 1; /* minimal timeout to avoid infinite blocking */
    } else {
        /* Indefinite blocking: wait until data arrives */
        to.ReadIntervalTimeout = MAXDWORD;
        to.ReadTotalTimeoutMultiplier = MAXDWORD;
        to.ReadTotalTimeoutConstant = MAXDWORD - 1;
        to.WriteTotalTimeoutMultiplier = 0;
        to.WriteTotalTimeoutConstant = 0; /* 0 means infinite wait in Comm API */
    }
    return SetCommTimeouts(handle, &to) ? CAIS_SERIAL_OK : CAIS_SERIAL_ERROR;
}

cais_serial_t *cais_serial_open(const char *path, const cais_serial_opts_t *opts, char *err_buf, size_t err_cap)
{
    HANDLE h;
    cais_serial_t *port;
    DCB dcb;
    cais_serial_opts_t default_opts;
    char target_path[64];
    const char *p = (path && path[0] != '\0') ? path : "COM1";

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (opts == NULL) {
        default_opts.baud = 9600;
        default_opts.data_bits = 8;
        default_opts.parity = 'n';
        default_opts.stop_bits = 1;
        default_opts.flow_control = 0; /* safe default: none */
        default_opts.rts = 1;
        default_opts.dtr = 1;
        default_opts.timeout_ms = -1; /* default: blocking indefinitely */
        opts = &default_opts;
    }

    if (strncmp(p, "\\\\.\\", 4) == 0 || strncmp(p, "\\\\?\\", 4) == 0) {
        strncpy(target_path, p, sizeof(target_path) - 1);
    } else {
        snprintf(target_path, sizeof(target_path), "\\\\.\\%s", p);
    }
    target_path[sizeof(target_path) - 1] = '\0';

    h = CreateFileA(target_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        cais_set_err_fmt(err_buf, err_cap, "CreateFileA(%s) failed (error %lu)", path,
                         (unsigned long)GetLastError());
        return NULL;
    }

    port = (cais_serial_t *)malloc(sizeof(cais_serial_t));
    if (port == NULL) {
        CloseHandle(h);
        cais_set_err(err_buf, err_cap, "Memory allocation failure");
        return NULL;
    }
    port->handle = h;
    port->timeout_ms = (opts != NULL) ? opts->timeout_ms : -1;

    memset(&port->orig_dcb, 0, sizeof(DCB));
    port->orig_dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h, &port->orig_dcb)) {
        cais_set_err(err_buf, err_cap, "GetCommState failed");
        CloseHandle(h);
        free(port);
        return NULL;
    }

    dcb = port->orig_dcb;
    dcb.BaudRate = (DWORD)opts->baud;
    dcb.ByteSize = (BYTE)opts->data_bits;

    /* Parity */
    if (opts->parity == 'e' || opts->parity == 'E') {
        dcb.fParity = TRUE;
        dcb.Parity = EVENPARITY;
    } else if (opts->parity == 'o' || opts->parity == 'O') {
        dcb.fParity = TRUE;
        dcb.Parity = ODDPARITY;
    } else {
        dcb.fParity = FALSE;
        dcb.Parity = NOPARITY;
    }

    /* Stop bits */
    if (opts->stop_bits == 2) {
        dcb.StopBits = TWOSTOPBITS;
    } else {
        dcb.StopBits = ONESTOPBIT;
    }

    /* Flow Control */
    if (opts->flow_control == 1) { /* hardware: RTS/CTS */
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
    } else if (opts->flow_control == 2) { /* software: XON/XOFF */
        dcb.fOutxCtsFlow = FALSE;
        dcb.fRtsControl = (opts->rts) ? RTS_CONTROL_ENABLE : RTS_CONTROL_DISABLE;
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
    } else { /* none / off */
        dcb.fOutxCtsFlow = FALSE;
        dcb.fRtsControl = (opts->rts) ? RTS_CONTROL_ENABLE : RTS_CONTROL_DISABLE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
    }

    /* Initial DTR line state */
    dcb.fDtrControl = (opts->dtr) ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;

    if (!SetCommState(h, &dcb)) {
        cais_set_err_fmt(err_buf, err_cap, "SetCommState failed (error %lu)",
                         (unsigned long)GetLastError());
        CloseHandle(h);
        free(port);
        return NULL;
    }

    /* Configure communication timeouts */
    cais_serial_apply_comm_timeouts(h, port->timeout_ms);

    /* Assert or clear RTS/DTR lines if specified */
    if (opts->rts >= 0) {
        EscapeCommFunction(h, opts->rts ? SETRTS : CLRRTS);
    }
    if (opts->dtr >= 0) {
        EscapeCommFunction(h, opts->dtr ? SETDTR : CLRDTR);
    }

    return port;
}

cais_serial_status_t cais_serial_set_rts(cais_serial_t *port, cais_serial_line_t state)
{
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) {
        return CAIS_SERIAL_ERROR;
    }
    return EscapeCommFunction(port->handle, (state == CAIS_SERIAL_LINE_ON || (int)state == 1) ? SETRTS : CLRRTS) ? CAIS_SERIAL_OK : CAIS_SERIAL_ERROR;
}

cais_serial_status_t cais_serial_set_dtr(cais_serial_t *port, cais_serial_line_t state)
{
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) {
        return CAIS_SERIAL_ERROR;
    }
    return EscapeCommFunction(port->handle, (state == CAIS_SERIAL_LINE_ON || (int)state == 1) ? SETDTR : CLRDTR) ? CAIS_SERIAL_OK : CAIS_SERIAL_ERROR;
}

cais_status_t cais_serial_write(cais_serial_t *port, const char *buf, size_t len, size_t *written_bytes)
{
    DWORD written = 0;
    size_t total = 0;

    if (written_bytes != NULL) {
        *written_bytes = 0;
    }

    if (port == NULL || port->handle == INVALID_HANDLE_VALUE || buf == NULL) {
        return CAIS_ERROR;
    }
    if (len == 0) {
        return CAIS_OK;
    }

    while (total < len) {
        DWORD chunk = (DWORD)(len - total);
        if (!WriteFile(port->handle, buf + total, chunk, &written, NULL)) {
            if (written_bytes != NULL) *written_bytes = total;
            return (total > 0) ? CAIS_OK : CAIS_ERROR;
        }
        if (written == 0) {
            break;
        }
        total += written;
    }

    if (written_bytes != NULL) {
        *written_bytes = total;
    }
    return (total > 0 || len == 0) ? CAIS_OK : CAIS_AGAIN;
}

cais_status_t cais_serial_read(cais_serial_t *port, char *buf, size_t max_len, size_t *read_bytes, int timeout_ms)
{
    DWORD read_len = 0;
    int need_restore = 0;

    if (read_bytes != NULL) {
        *read_bytes = 0;
    }

    if (port == NULL || port->handle == INVALID_HANDLE_VALUE || buf == NULL || max_len == 0) {
        return CAIS_ERROR;
    }

    if (timeout_ms >= 0 && timeout_ms != port->timeout_ms) {
        if (cais_serial_apply_comm_timeouts(port->handle, timeout_ms) == CAIS_SERIAL_OK) {
            need_restore = 1;
        }
    }

    if (!ReadFile(port->handle, buf, (DWORD)max_len, &read_len, NULL)) {
        if (need_restore) {
            cais_serial_apply_comm_timeouts(port->handle, port->timeout_ms);
        }
        return CAIS_ERROR;
    }

    if (need_restore) {
        cais_serial_apply_comm_timeouts(port->handle, port->timeout_ms);
    }

    if (read_len == 0) {
        return CAIS_AGAIN; /* Timeout */
    }

    if (read_bytes != NULL) {
        *read_bytes = (size_t)read_len;
    }
    return CAIS_OK;
}

cais_serial_status_t cais_serial_set_timeout(cais_serial_t *port, int timeout_ms)
{
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) {
        return CAIS_SERIAL_ERROR;
    }
    if (cais_serial_apply_comm_timeouts(port->handle, timeout_ms) != CAIS_SERIAL_OK) {
        return CAIS_SERIAL_ERROR;
    }
    port->timeout_ms = timeout_ms;
    return CAIS_SERIAL_OK;
}

cais_line_state_t cais_serial_get_cts(cais_serial_t *port)
{
    DWORD modem_stat = 0;
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) return CAIS_LINE_ERROR;
    if (!GetCommModemStatus(port->handle, &modem_stat)) return CAIS_LINE_ERROR;
    return (modem_stat & MS_CTS_ON) ? CAIS_LINE_HIGH : CAIS_LINE_LOW;
}

cais_line_state_t cais_serial_get_dsr(cais_serial_t *port)
{
    DWORD modem_stat = 0;
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) return CAIS_LINE_ERROR;
    if (!GetCommModemStatus(port->handle, &modem_stat)) return CAIS_LINE_ERROR;
    return (modem_stat & MS_DSR_ON) ? CAIS_LINE_HIGH : CAIS_LINE_LOW;
}

cais_line_state_t cais_serial_get_cd(cais_serial_t *port)
{
    DWORD modem_stat = 0;
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) return CAIS_LINE_ERROR;
    if (!GetCommModemStatus(port->handle, &modem_stat)) return CAIS_LINE_ERROR;
    return (modem_stat & MS_RLSD_ON) ? CAIS_LINE_HIGH : CAIS_LINE_LOW;
}

cais_line_state_t cais_serial_get_ri(cais_serial_t *port)
{
    DWORD modem_stat = 0;
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) return CAIS_LINE_ERROR;
    if (!GetCommModemStatus(port->handle, &modem_stat)) return CAIS_LINE_ERROR;
    return (modem_stat & MS_RING_ON) ? CAIS_LINE_HIGH : CAIS_LINE_LOW;
}

cais_serial_status_t cais_serial_drain(cais_serial_t *port)
{
    if (port == NULL || port->handle == INVALID_HANDLE_VALUE) {
        return CAIS_SERIAL_ERROR;
    }
    return FlushFileBuffers(port->handle) ? CAIS_SERIAL_OK : CAIS_SERIAL_ERROR;
}

void cais_serial_close(cais_serial_t *port)
{
    if (port != NULL) {
        if (port->handle != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(port->handle);
            SetCommState(port->handle, &port->orig_dcb);
            CloseHandle(port->handle);
            port->handle = INVALID_HANDLE_VALUE;
        }
        free(port);
    }
}

#else
/* ISO C forbids an empty translation unit */
typedef int make_iso_compilers_happy_serial_windows_nt;
#endif /* CAIS_IS_WINDOWS_NT */
