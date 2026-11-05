# Serial Port (UART) (`cais.serial`)

The `serial` subsystem provides complete RS-232 UART communication, hardware flow control (RTS/CTS), software flow control (XON/XOFF), modem line monitoring, and hardware buffer flushing.

---

## 1. Constants & Enumerations

### Baud Rates (`cais_serial_baud_t`)

| C Constant | Lua Field (`cais.serial.baud`) | Integer Baud |
| :--- | :--- | :---: |
| `CAIS_SERIAL_BAUD_300` | `B300` | `300` |
| `CAIS_SERIAL_BAUD_1200` | `B1200` | `1200` |
| `CAIS_SERIAL_BAUD_2400` | `B2400` | `2400` |
| `CAIS_SERIAL_BAUD_4800` | `B4800` | `4800` |
| `CAIS_SERIAL_BAUD_9600` | `B9600` | `9600` |
| `CAIS_SERIAL_BAUD_19200` | `B19200` | `19200` |
| `CAIS_SERIAL_BAUD_38400` | `B38400` | `38400` |
| `CAIS_SERIAL_BAUD_57600` | `B57600` | `57600` |
| `CAIS_SERIAL_BAUD_115200` | `B115200` | `115200` |

### Data Bits (`cais_serial_data_bits_t`)

| C Constant | Lua Field (`cais.serial.data_bits`) | Bit Count |
| :--- | :--- | :---: |
| `CAIS_SERIAL_DATA_5` | `D5` | `5` |
| `CAIS_SERIAL_DATA_6` | `D6` | `6` |
| `CAIS_SERIAL_DATA_7` | `D7` | `7` |
| `CAIS_SERIAL_DATA_8` | `D8` | `8` |

### Parity (`cais_serial_parity_t`)

| C Constant | Lua Field (`cais.serial.parity`) | Character / String |
| :--- | :--- | :---: |
| `CAIS_SERIAL_PARITY_NONE` | `NONE` | `'n'` / `"none"` |
| `CAIS_SERIAL_PARITY_EVEN` | `EVEN` | `'e'` / `"even"` |
| `CAIS_SERIAL_PARITY_ODD` | `ODD` | `'o'` / `"odd"` |
| `CAIS_SERIAL_PARITY_MARK` | `MARK` | `'m'` / `"mark"` |
| `CAIS_SERIAL_PARITY_SPACE` | `SPACE` | `'s'` / `"space"` |

### Stop Bits (`cais_serial_stop_bits_t`)

| C Constant | Lua Field (`cais.serial.stop_bits`) | Count |
| :--- | :--- | :---: |
| `CAIS_SERIAL_STOP_1` | `S1` | `1` |
| `CAIS_SERIAL_STOP_2` | `S2` | `2` |

### Flow Control (`cais_serial_flow_t`)

| C Constant | Lua Field (`cais.serial.flow`) | Mode Description |
| :--- | :--- | :--- |
| `CAIS_SERIAL_FLOW_NONE` | `NONE` (Default) | No flow control. Safe default preventing cable lockup. |
| `CAIS_SERIAL_FLOW_HARDWARE` | `HARDWARE` | Hardware RTS/CTS handshake pins. |
| `CAIS_SERIAL_FLOW_SOFTWARE` | `SOFTWARE` | Software in-band XON/XOFF flow control. |

### Signal Line Status (`cais_line_state_t`)

| C Constant | Lua Field (`cais.serial.line`) | Value | Description |
| :--- | :--- | :---: | :--- |
| `CAIS_LINE_LOW` | `LOW` | `0` | Physical line is DEASSERTED / LOW |
| `CAIS_LINE_HIGH` | `HIGH` | `1` | Physical line is ASSERTED / HIGH |
| `CAIS_LINE_ERROR` | `ERROR` | `-1` | Hardware fault, pin unsupported, or invalid handle |

---

## 2. C89 Function Reference

### `cais_serial_open`
```c
cais_serial_t *cais_serial_open(
    const char *path, 
    const cais_serial_opts_t *opts, 
    char *err_buf, 
    size_t err_cap
);
```
Opens a physical UART or virtual COM device.
* **`cais_serial_opts_t` Fields:**
  ```c
  typedef struct {
      int baud;         /* cais_serial_baud_t (default: 9600) */
      int data_bits;    /* cais_serial_data_bits_t (5, 6, 7, 8) */
      char parity;      /* cais_serial_parity_t ('n', 'e', 'o', 'm', 's') */
      int stop_bits;    /* cais_serial_stop_bits_t (1, 2) */
      int flow_control; /* cais_serial_flow_t (0: none, 1: hardware, 2: software) */
      int rts;          /* cais_serial_line_t (-1: ignore, 0: off, 1: on) */
      int dtr;          /* cais_serial_line_t (-1: ignore, 0: off, 1: on) */
      int timeout_ms;   /* 0 = non-blocking, >0 = timeout ms, <0 = indefinite */
  } cais_serial_opts_t;
  ```
* **Returns:** Active handle to `cais_serial_t`, or `NULL` on failure.

---

### `cais_serial_write`
```c
cais_status_t cais_serial_write(
    cais_serial_t *port, 
    const char *buf, 
    size_t len, 
    size_t *written_bytes
);
```
Writes bytes to the serial transmitter queue.
* **Returns:**
  * `CAIS_OK`: Bytes queued successfully (*written_bytes populated).
  * `CAIS_AGAIN`: Timeout expired before any bytes could be accepted.
  * `CAIS_ERROR`: I/O error or disconnected device.

---

### `cais_serial_read`
```c
cais_status_t cais_serial_read(
    cais_serial_t *port, 
    char *buf, 
    size_t max_len, 
    size_t *read_bytes, 
    int timeout_ms
);
```
Reads available bytes from receiver FIFO.
* **Parameters:**
  * `timeout_ms`: If `< 0`, uses port default timeout. If `0`, non-blocking check. If `> 0`, overrides timeout for this call.
* **Returns:** `CAIS_OK` on data, `CAIS_AGAIN` on timeout, `CAIS_ERROR` on failure.

---

### `cais_serial_drain`
```c
cais_serial_status_t cais_serial_drain(cais_serial_t *port);
```
Blocks execution until all pending bytes in the hardware transmitter FIFO and UART shift register have physically been emitted onto the wire.

---

### Modem Status & Control Functions

```c
/* Control output lines */
cais_serial_status_t cais_serial_set_rts(cais_serial_t *port, cais_serial_line_t state);
cais_serial_status_t cais_serial_set_dtr(cais_serial_t *port, cais_serial_line_t state);

/* Query input lines (returns CAIS_LINE_HIGH, CAIS_LINE_LOW, or CAIS_LINE_ERROR) */
cais_line_state_t cais_serial_get_cts(cais_serial_t *port);
cais_line_state_t cais_serial_get_dsr(cais_serial_t *port);
cais_line_state_t cais_serial_get_cd(cais_serial_t *port);
cais_line_state_t cais_serial_get_ri(cais_serial_t *port);
```

---

## 3. Lua API Reference

### Opening Port

```lua
local serial = require("cais.serial")

-- Options Table:
local port, err = serial.open("/dev/ttyUSB0", {
    baud = serial.baud.B115200,
    data_bits = 8,
    parity = "none",
    stop_bits = 1,
    flow = "none",
    timeout = 1000
})
```

### Port Methods

* `port:read([max_len, timeout_ms])`: Reads up to `max_len` bytes. Returns `data_str` or `nil, err`.
* `port:write(str)`: Writes bytes. Returns `written_count` or `nil, err`.
* `port:drain()`: Flushes physical hardware FIFO. Returns `true` or `nil, err`.
* `port:set_timeout(timeout_ms)`: Sets default timeout.
* `port:set_rts(bool)` / `port:set_dtr(bool)`: Asserts/deasserts RTS/DTR output lines.
* `port:get_cts()`, `port:get_dsr()`, `port:get_cd()`, `port:get_ri()`: Returns `serial.line.HIGH` or `serial.line.LOW`. (Predicate aliases `port:is_cts()`, `port:is_dsr()`, `port:is_cd()`, `port:is_ri()` are also provided).
* `port:close()`: Closes device node immediately.
