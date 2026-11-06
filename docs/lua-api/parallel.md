# cais.parallel

The `cais.parallel` module provides physical IEEE 1284 / Centronics parallel port control, bidirectional data strobing, and hardware status pin interrogation.

---

## Requiring the Module

```lua
local parallel = require("cais.parallel")
```

---

## Enumeration Tables

### `parallel.mode`

* `parallel.mode.SPP`: Standard Parallel Port (classic Centronics output-only unidirectional bus).
* `parallel.mode.EPP`: Enhanced Parallel Port (high-speed bidirectional bus).
* `parallel.mode.ECP`: Extended Capability Port (hardware FIFO and Run-Length Encoding).

---

## Opening a Parallel Port

### `parallel.open(path, opts?)`

Open a parallel port device (e.g. `"/dev/parport0"` on Linux, `"LPT1"` on DOS/Windows).

```lua
local port, err = parallel.open("/dev/parport0", {
    mode = parallel.mode.SPP
})
if not port then
    error("Failed to open parallel port: " .. tostring(err))
end
```

---

## `ParallelPort` Userdata Methods

### `port:write(data)`

Transmit a string of bytes over the 8-bit parallel data lines (`D0..D7`), pulsing `STROBE#` low for each byte.

* **Returns:** `integer` (bytes transferred) or `nil, err`.

```lua
local written, err = port:write("HELLO PRINTER\r\n")
```

### `port:read(max_bytes?)`

Read bytes from peripheral (requires bidirectional mode: EPP or ECP).

* **Returns:** `string` or `nil, err`.

### `port:status()` / `port:get_status()`

Retrieve raw status register integer byte from port base address `+1`.

### `port:set_mode(mode)`

Switch operating mode (`parallel.mode.SPP`, `EPP`, `ECP`).

### `port:drain()` / `port:flush()`

Block execution until all hardware FIFO bytes have cleared the bus.

### `port:close()`

Explicitly release parallel port device handle.

---

## Hardware Pin Predicates

The `ParallelPort` object provides dedicated boolean methods that read hardware status lines in real time:

| Method             | Pin Name      |    State    | Description                                                            |
| :----------------- | :------------ | :---------: | :--------------------------------------------------------------------- |
| `port:is_busy()`   | **BUSY**      | Active High | True if peripheral is processing, buffer is full, or device is offline |
| `port:is_ack()`    | **ACK#**      | Active Low  | True if peripheral acknowledged byte receipt with handshake pulse      |
| `port:is_pe()`     | **Paper End** | Active High | True if printer is out of paper (alias: `is_paper_out()`)              |
| `port:is_online()` | **Select**    | Active High | True if peripheral is selected and powered online (alias: `is_slct()`) |
| `port:is_fault()`  | **FAULT#**    | Active Low  | True if peripheral reported error condition, head jam, or cover open   |

---

## Handshake Loop Example

```lua
local parallel = require("cais.parallel")
local port = parallel.open("/dev/parport0")

function print_character(char_byte)
    -- 1. Wait until peripheral is not busy
    while port:is_busy() do
        if port:is_fault() then
            error("Printer reported hardware fault")
        end
    end

    -- 2. Transmit byte with automatic hardware STROBE# pulse
    port:write(char_byte)
end

print_character("A")
port:close()
```
