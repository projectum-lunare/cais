# cais.serial

The `cais.serial` module provides physical RS-232 / UART serial communication with baud rate selection, hardware flow control, parity validation, and modem line status interrogation.

---

## Requiring the Module

```lua
local serial = require("cais.serial")
```

---

## Enumeration Tables

### `serial.baud`

Constants for standard and high-speed baud rates:
`B50`, `B75`, `B110`, `B134`, `B150`, `B200`, `B300`, `B600`, `B1200`, `B1800`, `B2400`, `B4800`, `B9600`, `B19200`, `B38400`, `B57600`, `B115200`, `B230400`, `B460800`, `B921600`.

### `serial.flow`

* `serial.flow.NONE`: No flow control.
* `serial.flow.HARDWARE`: Hardware RTS/CTS handshake lines.
* `serial.flow.XONXOFF`: Software in-band XON/XOFF flow control.

### `serial.parity`

* `serial.parity.NONE`: No parity bit.
* `serial.parity.EVEN`: Even parity.
* `serial.parity.ODD`: Odd parity.
* `serial.parity.MARK`: Mark parity bit (always 1).
* `serial.parity.SPACE`: Space parity bit (always 0).

### `serial.stop`

* `serial.stop.ONE`: 1 stop bit.
* `serial.stop.TWO`: 2 stop bits.

### `serial.data`

* `serial.data.B5`, `B6`, `B7`, `B8`: Data word lengths from 5 to 8 bits.

---

## Opening a Serial Port

### `serial.open(path, opts_or_baud, ...)`

Constructors accept an options table or positional arguments:

=== "Options Table (Recommended)"

    ```lua
    local port, err = serial.open("/dev/ttyUSB0", {
        baud      = serial.baud.B115200, -- or integer: 115200
        data_bits = 8,
        parity    = "none",              -- "none", "even", "odd", "mark", "space"
        stop_bits = 1,
        flow      = "none",              -- "none", "hardware", "xonxoff"
        timeout   = 1000                 -- 1000ms read/write timeout
    })
    ```

=== "Positional Arguments"

    ```lua
    -- signature: (path, baud, data_bits, parity_str, stop_bits, timeout_ms)
    local port, err = serial.open("/dev/ttyUSB0", 115200, 8, "N", 1, 1000)
    ```

---

## `SerialPort` Userdata Methods

### `port:write(data)`

Transmit raw byte string to the serial device.

* **Returns:** `integer` (bytes written) or `nil, err`.

### `port:read(max_bytes?)`

Read incoming bytes from serial FIFO until timeout expires or buffer fills.

* **Returns:** `string` or `nil, err`.

### `port:drain()` / `port:flush()`

Block until all written data has been physically transmitted over the wire.

### `port:set_timeout(timeout_ms)`

Update read and write timeout dynamically in milliseconds (`-1` for blocking).

### `port:set_rts(active)`

Assert (`true`) or deassert (`false`) the Request To Send (RTS) line.

### `port:set_dtr(active)`

Assert (`true`) or deassert (`false`) the Data Terminal Ready (DTR) line.

---

## Modem Line Status Predicates

Inspect hardware control lines directly from the RS-232 transceiver:

| Method | Pin Name | Description |
| :--- | :--- | :--- |
| `port:is_cts()` | **Clear To Send** | True if peripheral is ready to accept transmitted data |
| `port:is_dsr()` | **Data Set Ready** | True if modem/peripheral is powered and online |
| `port:is_cd()` | **Carrier Detect** | True if data carrier connection is established |
| `port:is_ri()` | **Ring Indicator** | True if incoming telephone/modem ring voltage is detected |

### `port:close()`

Explicitly release and close the serial port device.
