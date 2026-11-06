# Serial Port (UART) (`cais.serial`)

The `serial` subsystem provides low-level and high-level control of physical and virtual RS-232, RS-422, and RS-485 serial communication interfaces.

It abstracts universal asynchronous receiver-transmitter (UART) hardware across modern operating systems, vintage personal computers, and microcontrollers, supporting arbitrary baud rates, framing formats (data bits, parity, stop bits), hardware flow control (RTS/CTS), software flow control (XON/XOFF), in-depth modem line monitoring, and hardware buffer flushing.

---

## ⚡ 1. Hardware Architecture & RS-232 Fundamentals

Serial communication transmits data sequentially, one bit at a time, over a pair of conducting lines (transmit **TX** and receive **RX**), referenced to common ground (**GND**).

### 1.1 UART Frame Structure

A UART frame consists of four consecutive phases:

1. **Start Bit:** The transmission line idles at logic `HIGH` (mark). A transition to logic `LOW` (space) for 1 bit-period signals the arrival of a new character.
2. **Data Bits:** 5, 6, 7, or 8 bits transmitted least-significant bit (LSB) first. The standard across modern systems is **8 data bits**.
3. **Parity Bit (Optional):** An error-detection bit computed from the data bits:
   * **None (`'n'`):** No parity bit transmitted.
   * **Even (`'e'`):** Parity bit is set such that total number of `1`s is even.
   * **Odd (`'o'`):** Parity bit is set such that total number of `1`s is odd.
   * **Mark (`'m'`):** Parity bit is permanently asserted (`1`).
   * **Space (`'s'`):** Parity bit is permanently deasserted (`0`).
4. **Stop Bits:** 1 or 2 bits holding the line at logic `HIGH` to allow the receiver's internal clock to re-synchronize before the next start bit.

> [!NOTE]
> The industry standard default configuration is **`8-N-1`** (8 data bits, No parity, 1 stop bit) at **9600** or **115200** baud.

---

### 1.2 Flow Control Modes

When the receiver's hardware or software buffers fill up, flow control prevents data loss:

* **None (`CAIS_SERIAL_FLOW_NONE` / `flow = "none"`):**
  No throttling. If the receiver cannot process incoming bytes fast enough, the UART receiver FIFO overflows and bytes are permanently dropped.
* **Hardware RTS/CTS (`CAIS_SERIAL_FLOW_HARDWARE` / `flow = "hardware"`):**
  Uses dedicated physical lines. The receiver deasserts **CTS** (Clear to Send) to pause the transmitter, and asserts it when buffer space is available. This is immune to payload data corruption and provides sub-millisecond reaction times.
* **Software XON/XOFF (`CAIS_SERIAL_FLOW_SOFTWARE` / `flow = "software"`):**
  In-band flow control using ASCII control codes: `DC3` (`0x13`, XOFF) pauses transmission, and `DC1` (`0x11`, XON) resumes. **Never use software flow control when transmitting binary data**, as payload bytes matching `0x11` or `0x13` will stall communication.

---

### 1.3 RS-232 Modem Pinout & Control Lines

Modern 9-pin (DE-9) and vintage 25-pin (DB-25) connectors expose hardware handshaking lines divided into output lines (driven by `cais`) and input lines (monitored by `cais`):

| Signal Name             |    Abbr.     | DE-9 Pin | DB-25 Pin | Direction | Description                                                     |
| :---------------------- | :----------: | :------: | :-------: | :-------: | :-------------------------------------------------------------- |
| **Transmit Data**       |    `TXD`     |  Pin 3   |   Pin 2   |  Output   | Outgoing serial bitstream from host to device                   |
| **Receive Data**        |    `RXD`     |  Pin 2   |   Pin 3   |   Input   | Incoming serial bitstream from device to host                   |
| **Request to Send**     |    `RTS`     |  Pin 7   |   Pin 4   |  Output   | Host asserts when ready to receive or initiate transfer         |
| **Clear to Send**       |    `CTS`     |  Pin 8   |   Pin 5   |   Input   | Peripheral asserts when ready to accept incoming bytes          |
| **Data Terminal Ready** |    `DTR`     |  Pin 4   |  Pin 20   |  Output   | Host asserts to signal it is powered on and online              |
| **Data Set Ready**      |    `DSR`     |  Pin 6   |   Pin 6   |   Input   | Modem/peripheral asserts when powered on and ready              |
| **Data Carrier Detect** | `CD` / `DCD` |  Pin 1   |   Pin 8   |   Input   | Modem asserts when an active phone/radio carrier is detected    |
| **Ring Indicator**      |     `RI`     |  Pin 9   |  Pin 22   |   Input   | Modem asserts when an incoming telephone ring voltage is sensed |
| **Signal Ground**       |    `GND`     |  Pin 5   |   Pin 7   |  Ground   | 0V reference potential for all logic levels                     |

---

## 🔌 2. Platform Device Path Conventions

Serial ports are addressed differently across operating system kernels:

| Operating System Family        | Device Path Syntax                                                            | Examples                                         |
| :----------------------------- | :---------------------------------------------------------------------------- | :----------------------------------------------- |
| **Linux**                      | `/dev/ttyS*` (onboard), `/dev/ttyUSB*` (FTDI/CH340), `/dev/ttyACM*` (CDC-ACM) | `/dev/ttyS0`, `/dev/ttyUSB0`, `/dev/ttyACM0`     |
| **macOS Darwin**               | `/dev/cu.usbserial-*`, `/dev/cu.usbmodem*` *(Call-Out devices)*               | `/dev/cu.usbserial-1420`, `/dev/cu.usbmodem1101` |
| **Windows NT / 2000..11**      | `COM1` .. `COM9`, or `\\.\COM10`..`\\.\COM256` for two-digit ports            | `COM1`, `COM3`, `\\.\COM12`                      |
| **Windows CE**                 | `COM1:`, `COM2:` (trailing colon required by WinCE driver)                    | `COM1:`, `COM2:`                                 |
| **FreeBSD / OpenBSD / NetBSD** | `/dev/cuaU*` (USB call-out) or `/dev/cuad*` / `/dev/cua00`                    | `/dev/cuaU0`, `/dev/cuad0`, `/dev/cua00`         |
| **MS-DOS / FreeDOS**           | `COM1`, `COM2`, `COM3`, `COM4`                                                | `COM1`, `COM2`                                   |
| **Commodore AmigaOS**          | `serial.device` (Unit number configured via options)                          | `serial.device`                                  |
| **Atari ST (TOS / FreeMiNT)**  | `AUX:`, `MODEM1`, `MODEM2`                                                    | `AUX:`, `MODEM1`                                 |
| **IBM OS/2 Warp**              | `COM1`, `COM2`, `COM3`, `COM4`                                                | `COM1`, `COM2`                                   |
| **Plan 9 from Bell Labs**      | `#t/eia1` (data), `#t/eia1ctl` (control)                                      | `/dev/eia1`, `#t/eia1`                           |
| **KolibriOS**                  | Direct UART port address                                                      | `0x3F8` (COM1), `0x2F8` (COM2)                   |

> [!TIP]
> On macOS and BSDs, **always use the call-out device** (`/dev/cu.*` or `/dev/cua*`) rather than the dial-in device (`/dev/tty.*`). Dial-in devices block waiting for the modem DCD line to be asserted before `open()` can return.

---

## ⚙️ 3. Constants & Enumerations

### 3.1 Standard Baud Rates (`cais_serial_baud_t`)

| C Constant                | Lua Field (`cais.serial.baud`) | Integer Value | Typical Application                            |
| :------------------------ | :----------------------------- | :-----------: | :--------------------------------------------- |
| `CAIS_SERIAL_BAUD_300`    | `B300`                         |     `300`     | Vintage acoustic modems, telemetry             |
| `CAIS_SERIAL_BAUD_1200`   | `B1200`                        |    `1200`     | Bell 212A modems, pager transmitters           |
| `CAIS_SERIAL_BAUD_2400`   | `B2400`                        |    `2400`     | Early dial-up BBS, industrial scales           |
| `CAIS_SERIAL_BAUD_4800`   | `B4800`                        |    `4800`     | NMEA-0183 GPS receivers (marine standard)      |
| `CAIS_SERIAL_BAUD_9600`   | `B9600`                        |    `9600`     | Standard default, microcontrollers, Arduino    |
| `CAIS_SERIAL_BAUD_19200`  | `B19200`                       |    `19200`    | Modbus RTU PLC industrial networks             |
| `CAIS_SERIAL_BAUD_38400`  | `B38400`                       |    `38400`    | High-speed GPS, dial-up v.32bis modems         |
| `CAIS_SERIAL_BAUD_57600`  | `B57600`                       |    `57600`    | 56k dial-up modems, vintage terminal servers   |
| `CAIS_SERIAL_BAUD_115200` | `B115200`                      |   `115200`    | Modern microcontrollers (ESP32, RP2040, STM32) |

On physical UART controllers (e.g. National Semiconductor 8250, 16450, 16550A), baud rates are generated from the hardware master clock (typically `1.8432 MHz`) via a 16-bit programmable divisor:

```text
Baud Rate = UART_CLOCK / (16 * Divisor)
Divisor   = UART_CLOCK / (16 * Baud Rate)
```

For example, generating 9600 baud yields `Divisor = 1843200 / (16 * 9600) = 12` (`0x000C`), while 115200 baud yields `Divisor = 1` (`0x0001`).

---

### 3.2 Data Bits (`cais_serial_data_bits_t`)

| C Constant           | Lua Field (`cais.serial.data_bits`) | Bits  | Notes                                      |
| :------------------- | :---------------------------------- | :---: | :----------------------------------------- |
| `CAIS_SERIAL_DATA_5` | `FIVE`                              |  `5`  | Baudot / Telex telegraphic encoding        |
| `CAIS_SERIAL_DATA_6` | `SIX`                               |  `6`  | Legacy mainframe telemetry                 |
| `CAIS_SERIAL_DATA_7` | `SEVEN`                             |  `7`  | Standard 7-bit ASCII text transmission     |
| `CAIS_SERIAL_DATA_8` | `EIGHT`                             |  `8`  | **Standard 8-bit binary octets (Default)** |

---

### 3.3 Parity (`cais_serial_parity_t`)

| C Constant                 | Lua Field (`cais.serial.parity`) |  ASCII Byte   | String Accepted in Lua |
| :------------------------- | :------------------------------- | :-----------: | :--------------------- |
| `CAIS_SERIAL_PARITY_NONE`  | `NONE`                           | `'n'` (`110`) | `"none"`, `"n"`        |
| `CAIS_SERIAL_PARITY_EVEN`  | `EVEN`                           | `'e'` (`101`) | `"even"`, `"e"`        |
| `CAIS_SERIAL_PARITY_ODD`   | `ODD`                            | `'o'` (`111`) | `"odd"`, `"o"`         |
| `CAIS_SERIAL_PARITY_MARK`  | `MARK`                           | `'m'` (`109`) | `"mark"`, `"m"`        |
| `CAIS_SERIAL_PARITY_SPACE` | `SPACE`                          | `'s'` (`115`) | `"space"`, `"s"`       |

---

### 3.4 Stop Bits (`cais_serial_stop_bits_t`)

| C Constant           | Lua Field (`cais.serial.stop_bits`) | Bits  | Typical Usage                                       |
| :------------------- | :---------------------------------- | :---: | :-------------------------------------------------- |
| `CAIS_SERIAL_STOP_1` | `ONE` / `STOP_1`                    |  `1`  | Universal standard across all modern links          |
| `CAIS_SERIAL_STOP_2` | `TWO` / `STOP_2`                    |  `2`  | Electromechanical teletypes, noisy industrial lines |

---

### 3.5 Flow Control (`cais_serial_flow_t`)

| C Constant                  | Lua Field (`cais.serial.flow`) | Mode Value | Description                                           |
| :-------------------------- | :----------------------------- | :--------: | :---------------------------------------------------- |
| `CAIS_SERIAL_FLOW_NONE`     | `NONE`                         |    `0`     | No flow control. Safe default preventing cable lockup |
| `CAIS_SERIAL_FLOW_HARDWARE` | `HARDWARE` / `RTS_CTS`         |    `1`     | Out-of-band hardware handshake pins (RTS/CTS)         |
| `CAIS_SERIAL_FLOW_SOFTWARE` | `SOFTWARE` / `XON_XOFF`        |    `2`     | In-band XON (`0x11`) / XOFF (`0x13`) software codes   |

---

### 3.6 Signal Line Status (`cais_line_state_t` / `cais_serial_line_t`)

| C Constant              | Lua Field (`cais.serial.line`) | Value | Description                                       |
| :---------------------- | :----------------------------- | :---: | :------------------------------------------------ |
| `CAIS_LINE_LOW` / `OFF` | `LOW` / `OFF`                  |  `0`  | Physical line is DEASSERTED / Logic LOW           |
| `CAIS_LINE_HIGH` / `ON` | `HIGH` / `ON`                  |  `1`  | Physical line is ASSERTED / Logic HIGH            |
| `CAIS_LINE_ERROR`       | `ERROR` / `IGNORE`             | `-1`  | Pin unsupported by driver, disconnected, or error |

---

## 🏛️ 4. ANSI C89 Function Reference

All public functions are declared in [`include/cais/serial.h`](file:///home/gustavo/Projetos/cais/include/cais/serial.h) and are strictly ANSI C89 compliant.

### 4.1 Opening and Configuring a Port

```c
cais_serial_t *cais_serial_open(
    const char *path, 
    const cais_serial_opts_t *opts, 
    char *err_buf, 
    size_t err_cap
);
```

Opens a physical UART or virtual COM device and applies line discipline parameters.

* **Parameters:**
  * `path`: Canonical platform device path (e.g. `"/dev/ttyUSB0"`, `"COM1"`).
  * `opts`: Pointer to configuration struct. If `NULL`, default parameters (`9600, 8-N-1, flow=none, blocking`) are applied.
  * `err_buf`: Optional pre-allocated buffer receiving human-readable error descriptions on failure (zero heap allocation).
  * `err_cap`: Capacity in bytes of `err_buf`.
* **The `cais_serial_opts_t` Structure:**
  ```c
  typedef struct cais_serial_opts {
      int  baud;         /* Baud rate: 9600, 115200, or cais_serial_baud_t */
      int  data_bits;    /* Data bits: 5, 6, 7, 8 (default: 8) */
      char parity;       /* Parity: 'n', 'e', 'o', 'm', 's' (default: 'n') */
      int  stop_bits;    /* Stop bits: 1 or 2 (default: 1) */
      int  flow_control; /* Flow mode: 0 (none), 1 (hardware), 2 (software) */
      int  rts;          /* Initial RTS line state: -1 (ignore), 0 (low), 1 (high) */
      int  dtr;          /* Initial DTR line state: -1 (ignore), 0 (low), 1 (high) */
      int  timeout_ms;   /* Timeout: <0 (blocking), 0 (non-blocking), >0 (milliseconds) */
  } cais_serial_opts_t;
  ```
* **Returns:** Active pointer to `cais_serial_t`, or `NULL` on failure.

---

### 4.2 Data Transmission & Reception

```c
cais_status_t cais_serial_write(
    cais_serial_t *port, 
    const char *buf, 
    size_t len, 
    size_t *written_bytes
);
```
Queues bytes into the serial transmitter queue.

* **Returns:**
  * `CAIS_OK`: Requested bytes accepted. `*written_bytes` is populated.
  * `CAIS_AGAIN`: Port is busy or timeout expired before any bytes could be accepted.
  * `CAIS_ERROR`: Hardware error, disconnected cable, or invalid descriptor.

```c
cais_status_t cais_serial_read(
    cais_serial_t *port, 
    char *buf, 
    size_t max_len, 
    size_t *read_bytes, 
    int timeout_ms
);
```
Reads incoming bytes from the receiver FIFO.

* **Timeout Behavior:**
  * `timeout_ms < 0`: Uses the default timeout configured on the port. If that is also `< 0`, blocks indefinitely until at least 1 byte arrives.
  * `timeout_ms == 0`: Instant non-blocking check. Returns `CAIS_AGAIN` immediately if the FIFO is empty.
  * `timeout_ms > 0`: Blocks until data arrives or the specified millisecond deadline expires.
* **Returns:** `CAIS_OK` on data, `CAIS_AGAIN` on timeout, `CAIS_ERROR` on failure.

---

### 4.3 Hardware Transmitter Drain

```c
cais_serial_status_t cais_serial_drain(cais_serial_t *port);
```
Blocks execution until all pending bytes in the operating system software buffer, the UART hardware transmitter FIFO, and the physical shift register have been transmitted over the wire.
> [!IMPORTANT]
> Always call `cais_serial_drain()` before toggling half-duplex RS-485 direction pins or closing a port, otherwise pending trailing bytes will be clipped.

---

### 4.4 Modem Pin Control & Queries

```c
/* Control host-driven output lines */
cais_serial_status_t cais_serial_set_rts(cais_serial_t *port, cais_serial_line_t state);
cais_serial_status_t cais_serial_set_dtr(cais_serial_t *port, cais_serial_line_t state);

/* Query peripheral-driven input lines */
cais_line_state_t cais_serial_get_cts(cais_serial_t *port);
cais_line_state_t cais_serial_get_dsr(cais_serial_t *port);
cais_line_state_t cais_serial_get_cd(cais_serial_t *port);
cais_line_state_t cais_serial_get_ri(cais_serial_t *port);
```
* **Line States Returned:**
  * `CAIS_LINE_HIGH` (`1`): Line is asserted (+3V..+15V on RS-232, 3.3V/5V on TTL).
  * `CAIS_LINE_LOW` (`0`): Line is deasserted (-3V..-15V on RS-232, 0V on TTL).
  * `CAIS_LINE_ERROR` (`-1`): Operation unsupported on host hardware or invalid port handle.

---

### 4.5 Port Teardown

```c
void cais_serial_close(cais_serial_t *port);
```
Restores original line discipline parameters (on POSIX), flushes pending buffers, and releases system file descriptors or device handles. Safe to pass `NULL`.

---

## 💻 5. Practical C89 Programming Examples

### Example 1: Robust NMEA-0183 GPS Receiver with Timed Line Reader

Reads lines from a standard GPS module emitting ASCII sentences (`$GPGGA`, `$GPRMC`) at 9600 baud, 8-N-1:

```c
#include <cais/serial.h>
#include <stdio.h>
#include <string.h>

int read_nmea_sentence(cais_serial_t *port, char *out_line, size_t max_len) {
    size_t count = 0;
    while (count + 1 < max_len) {
        char ch;
        size_t n = 0;
        /* Read single character with 1000ms timeout */
        cais_status_t st = cais_serial_read(port, &ch, 1, &n, 1000);
        if (st == CAIS_AGAIN) {
            fprintf(stderr, "Warning: GPS read timeout expired.\n");
            return -1;
        }
        if (st != CAIS_OK || n == 0) {
            return -1;
        }

        if (ch == '\n') {
            out_line[count] = '\0';
            return 0;
        }
        if (ch != '\r') {
            out_line[count++] = ch;
        }
    }
    out_line[count] = '\0';
    return 0;
}

int main(void) {
    char err[128];
    char line[256];
    cais_serial_opts_t opts;
    cais_serial_t *port;

    memset(&opts, 0, sizeof(opts));
    opts.baud = CAIS_SERIAL_BAUD_9600;
    opts.data_bits = CAIS_SERIAL_DATA_8;
    opts.parity = CAIS_SERIAL_PARITY_NONE;
    opts.stop_bits = CAIS_SERIAL_STOP_1;
    opts.flow_control = CAIS_SERIAL_FLOW_NONE;
    opts.timeout_ms = 1500;

    port = cais_serial_open("/dev/ttyUSB0", &opts, err, sizeof(err));
    if (!port) {
        fprintf(stderr, "Failed to open GPS: %s\n", err);
        return 1;
    }

    printf("GPS receiver connected. Listening for NMEA sentences...\n");
    while (read_nmea_sentence(port, line, sizeof(line)) == 0) {
        if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GPRMC", 6) == 0) {
            printf("NMEA: %s\n", line);
        }
    }

    cais_serial_close(port);
    return 0;
}
```

---

### Example 2: Industrial Modbus RTU Packet Transmitter with Hardware RTS/CTS

Transmits a Modbus query frame, waits for the hardware transmitter FIFO to drain completely, and verifies the peripheral is ready:

```c
#include <cais/serial.h>
#include <stdio.h>

int main(void) {
    char err[128];
    cais_serial_opts_t opts;
    cais_serial_t *port;
    /* Modbus RTU Read Holding Registers (Slave 1, Reg 0x0000, Count 2, CRC) */
    const unsigned char query[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B };
    size_t written = 0;

    opts.baud = CAIS_SERIAL_BAUD_19200;
    opts.data_bits = CAIS_SERIAL_DATA_8;
    opts.parity = CAIS_SERIAL_PARITY_EVEN; /* Modbus standard specifies Even parity */
    opts.stop_bits = CAIS_SERIAL_STOP_1;
    opts.flow_control = CAIS_SERIAL_FLOW_HARDWARE; /* Hardware RTS/CTS */
    opts.rts = CAIS_SERIAL_LINE_ON;
    opts.dtr = CAIS_SERIAL_LINE_ON;
    opts.timeout_ms = 500;

    port = cais_serial_open("/dev/ttyS0", &opts, err, sizeof(err));
    if (!port) {
        fprintf(stderr, "Failed to open PLC port: %s\n", err);
        return 1;
    }

    /* Verify PLC hardware handshake CTS pin before sending */
    if (cais_serial_get_cts(port) != CAIS_LINE_HIGH) {
        fprintf(stderr, "Warning: CTS line is LOW (PLC not ready).\n");
    }

    if (cais_serial_write(port, (const char *)query, sizeof(query), &written) == CAIS_OK) {
        /* Guarantee all bytes are physically emitted before continuing */
        cais_serial_drain(port);
        printf("Sent %lu bytes of Modbus frame.\n", (unsigned long)written);
    }

    cais_serial_close(port);
    return 0;
}
```

---

## 🌙 6. Lua API Reference & Idioms (Lua >= 5.4)

The serial driver is loaded via `require("cais.serial")` or accessed via the umbrella module `cais.serial`.

### 6.1 Constructor (`serial.open`)

Supports both an ergonomic **options table** and **positional arguments**:

```lua
local serial = require("cais.serial")

-- Table Constructor:
local port, err = serial.open("/dev/ttyUSB0", {
    baud = 115200,          -- integer or serial.baud.B115200 (default: 9600)
    data_bits = 8,          -- 5, 6, 7, 8 (default: 8)
    parity = "none",        -- "none", "even", "odd", "mark", "space"
    stop_bits = 1,          -- 1 or 2 (default: 1)
    flow = "none",          -- "none", "hardware", "software"
    rts = true,             -- initial RTS output line state
    dtr = true,             -- initial DTR output line state
    timeout = 1000          -- timeout in ms (<0: block, 0: non-block, >0: timed)
})

-- Positional Constructor:
-- open(path, baud, data_bits, parity, stop_bits, flow, rts, dtr, timeout)
local port, err = serial.open("COM1", 9600, 8, "n", 1, "none", true, true, 500)
```

---

### 6.2 Port Methods

| Method                         | Signatures                                     | Description                                                                                                     |
| :----------------------------- | :--------------------------------------------- | :-------------------------------------------------------------------------------------------------------------- |
| `port:read`                    | `port:read([max_len = 4096, timeout_ms = -1])` | Reads incoming bytes. Returns `data_string` on success, or `nil, err` on timeout/failure.                       |
| `port:write`                   | `port:write(data_str)`                         | Transmits bytes. Returns `written_count` on success, or `nil, err`.                                             |
| `port:drain`                   | `port:drain()`                                 | Flushes hardware UART transmitter FIFO. Returns `true` or `false, err`.                                         |
| `port:set_timeout`             | `port:set_timeout(timeout_ms)`                 | Configures default timeout in milliseconds.                                                                     |
| `port:set_rts`                 | `port:set_rts(bool)`                           | Asserts (`true`) or deasserts (`false`) RTS output pin.                                                         |
| `port:set_dtr`                 | `port:set_dtr(bool)`                           | Asserts (`true`) or deasserts (`false`) DTR output pin.                                                         |
| `port:get_cts` / `port:is_cts` | `port:get_cts()`                               | Returns boolean state (`true` if asserted, `false` if deasserted) or `nil, err`.                                |
| `port:get_dsr` / `port:is_dsr` | `port:get_dsr()`                               | Returns boolean state of DSR pin.                                                                               |
| `port:get_cd` / `port:is_cd`   | `port:get_cd()`                                | Returns boolean state of Carrier Detect pin.                                                                    |
| `port:get_ri` / `port:is_ri`   | `port:get_ri()`                                | Returns boolean state of Ring Indicator pin.                                                                    |
| `port:close`                   | `port:close()`                                 | Closes device handle immediately. (Also executed automatically during Lua garbage collection or via `<close>`). |

---

### Example 1: Microcontroller Telemetry Collector (ESP32 / Arduino)

```lua
local serial = require("cais.serial")

local port, err = serial.open("/dev/ttyUSB0", {
    baud = 115200,
    timeout = 2000
})

if not port then
    io.stderr:write("Could not open serial port: " .. tostring(err) .. "\n")
    return
end

print("Connected to microcontroller at 115200 baud.")

-- Request sensor metrics
port:write("GET:TELEMETRY\n")
port:drain()

-- Read incoming payload
local response, read_err = port:read(1024)
if response then
    print("Received payload: " .. response)
else
    print("Read error: " .. tostring(read_err))
end

port:close()
```

---

### Example 2: GSM / AT Modem Commander with DTR Reset & Ring Detection

```lua
local serial = require("cais.serial")

local modem, err = serial.open("/dev/ttyUSB2", {
    baud = 115200,
    rts = true,
    dtr = true,
    timeout = 1000
})

if not modem then
    error("Modem open failed: " .. tostring(err))
end

-- Hardware reset pulse using DTR line
modem:set_dtr(false)
os.execute("sleep 0.1")
modem:set_dtr(true)

-- Send Hayes AT ping
modem:write("AT\r\n")
modem:drain()

local reply = modem:read(64)
print("Modem response: " .. tostring(reply))

-- Check if phone line is ringing
if modem:is_ri() then
    print("INCOMING CALL DETECTED! (RI line asserted)")
end

modem:close()
```