---@meta cais.serial

---
---# cais.serial
---
---RS-232 / UART physical serial port driver, hardware flow control,
---and modem control line inspection.
---
local serial = {}

---@class CaisSerialBaud
---@field B50 integer
---@field B75 integer
---@field B110 integer
---@field B134 integer
---@field B150 integer
---@field B200 integer
---@field B300 integer
---@field B600 integer
---@field B1200 integer
---@field B1800 integer
---@field B2400 integer
---@field B4800 integer
---@field B9600 integer
---@field B19200 integer
---@field B38400 integer
---@field B57600 integer
---@field B115200 integer
---@field B230400 integer
---@field B460800 integer
---@field B921600 integer
serial.baud = {}

---@class CaisSerialFlow
---@field NONE integer No flow control
---@field HARDWARE integer Hardware RTS/CTS flow control
---@field XONXOFF integer Software XON/XOFF flow control
serial.flow = {}

---@class CaisSerialParity
---@field NONE integer No parity bit
---@field ODD integer Odd parity
---@field EVEN integer Even parity
---@field MARK integer Mark parity
---@field SPACE integer Space parity
serial.parity = {}

---@class CaisSerialStop
---@field ONE integer 1 stop bit
---@field TWO integer 2 stop bits
serial.stop = {}

---@class CaisSerialData
---@field B5 integer 5 data bits
---@field B6 integer 6 data bits
---@field B7 integer 7 data bits
---@field B8 integer 8 data bits
serial.data = {}

---@class CaisSerialOptions
---@field baud? integer Baud rate (e.g. `cais.serial.baud.B115200` or `115200`)
---@field data_bits? integer Number of data bits (5, 6, 7, 8)
---@field parity? integer|"none"|"even"|"odd"|"mark"|"space"|"N"|"E"|"O" Parity configuration
---@field stop_bits? integer Stop bits (1, 2)
---@field flow? integer|"none"|"hardware"|"xonxoff" Flow control mode
---@field timeout? integer I/O timeout in milliseconds (-1 for blocking)

---@class CaisSerialPort
local SerialPort = {}

---Write byte buffer to serial transmitter.
---@param data string Bytes to send
---@return integer|nil bytes_written, string? err
function SerialPort:write(data) end

---Read incoming bytes from serial receiver.
---@param max_bytes? integer Buffer capacity (default: 4096)
---@return string|nil data, string? err
function SerialPort:read(max_bytes) end

---Configure serial port read/write timeout.
---@param timeout_ms integer Timeout in milliseconds (-1 for blocking, 0 for non-blocking)
---@return boolean|nil success, string? err
function SerialPort:set_timeout(timeout_ms) end

---Wait until all output written to port has been physically transmitted.
---@return boolean|nil success, string? err
function SerialPort:drain() end

---Alias for `drain()`.
---@return boolean|nil success, string? err
function SerialPort:flush() end

---Assert or deassert Request To Send (RTS) modem pin.
---@param active boolean True to assert high, false to deassert low
---@return boolean|nil success, string? err
function SerialPort:set_rts(active) end

---Assert or deassert Data Terminal Ready (DTR) modem pin.
---@param active boolean True to assert high, false to deassert low
---@return boolean|nil success, string? err
function SerialPort:set_dtr(active) end

---Check if Clear To Send (CTS) line is active.
---@return boolean is_cts
function SerialPort:is_cts() end

---Check if Data Set Ready (DSR) line is active.
---@return boolean is_dsr
function SerialPort:is_dsr() end

---Check if Carrier Detect (CD) line is active.
---@return boolean is_cd
function SerialPort:is_cd() end

---Check if Ring Indicator (RI) line is active.
---@return boolean is_ri
function SerialPort:is_ri() end

---Explicitly close serial port descriptor.
---@return boolean|nil success, string? err
function SerialPort:close() end

---Open a serial device with configuration table or positional arguments.
---@overload fun(path: string, opts?: CaisSerialOptions): CaisSerialPort|nil, string?
---@overload fun(path: string, baud?: integer, data_bits?: integer, parity?: string|integer, stop_bits?: integer, timeout?: integer): CaisSerialPort|nil, string?
---@param path string Serial device file (e.g. "/dev/ttyUSB0", "COM1")
---@param opts_or_baud? CaisSerialOptions|integer Options table or baud rate
---@return CaisSerialPort|nil port, string? err
function serial.open(path, opts_or_baud, ...) end

return serial
