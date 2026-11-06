---@meta cais.socket

---
---# cais.socket
---
---Network sockets, TCP client/server, UDP datagrams, non-blocking I/O,
---and multi-descriptor multiplexing (`select()`).
---
local socket = {}

---@class CaisSocketMode
---@field BLOCKING integer Blocking I/O mode
---@field NONBLOCKING integer Non-blocking I/O mode
socket.mode = {}

---@class CaisSocketType
---@field STREAM integer TCP streaming socket
---@field DGRAM integer UDP datagram socket
socket.type = {}

---@class CaisSocketEvent
---@field READ integer Poll read readiness bitmask
---@field WRITE integer Poll write readiness bitmask
---@field ERROR integer Poll error condition bitmask
socket.event = {}

---@class CaisSocket
local Socket = {}

---Send byte payload over connected stream.
---@param data string Data buffer to transmit
---@return integer|nil bytes_sent, string? err
function Socket:send(data) end

---Alias for `send()`.
---@param data string Data buffer to transmit
---@return integer|nil bytes_sent, string? err
function Socket:write(data) end

---Receive incoming payload from socket stream.
---@param max_bytes? integer Maximum bytes to read (default: 4096)
---@return string|nil data, string? err
function Socket:recv(max_bytes) end

---Alias for `recv()`.
---@param max_bytes? integer Maximum bytes to read (default: 4096)
---@return string|nil data, string? err
function Socket:read(max_bytes) end

---Send datagram payload to specific destination address.
---@param data string Data payload
---@param host string Target hostname or IP
---@param port integer Target port
---@return integer|nil bytes_sent, string? err
function Socket:sendto(data, host, port) end

---Receive datagram payload along with sender address.
---@param max_bytes? integer Maximum buffer capacity
---@return string|nil data, string? sender_host, integer? sender_port, string? err
function Socket:recvfrom(max_bytes) end

---Bind listening socket to address and port.
---@param host string Host address (e.g. "0.0.0.0")
---@param port integer Port number
---@return boolean|nil success, string? err
function Socket:bind(host, port) end

---Connect client socket to remote host and port.
---@param host string Target hostname or IP
---@param port integer Target port number
---@return boolean|nil success, string? err
function Socket:connect(host, port) end

---Accept incoming connection on listening socket.
---@return CaisSocket|nil client_sock, string? client_ip, integer? client_port, string? err
function Socket:accept() end

---Configure socket blocking behavior.
---@param mode integer `cais.socket.mode.BLOCKING` or `cais.socket.mode.NONBLOCKING`
---@return boolean|nil success, string? err
function Socket:set_mode(mode) end

---Toggle blocking flag on socket.
---@param blocking boolean True for blocking, false for non-blocking
---@return boolean|nil success, string? err
function Socket:set_blocking(blocking) end

---Retrieve current socket mode.
---@return integer mode
function Socket:get_mode() end

---Check whether socket is in blocking mode.
---@return boolean is_blocking
function Socket:is_blocking() end

---Check whether socket is in non-blocking mode.
---@return boolean is_nonblocking
function Socket:is_nonblocking() end

---Set synchronous socket I/O timeout in milliseconds.
---@param timeout_ms integer Timeout duration (-1 for infinite, 0 for immediate)
---@return boolean|nil success, string? err
function Socket:set_timeout(timeout_ms) end

---Wait for readiness events on this socket.
---@param events integer Event bitmask (`cais.socket.event.READ`, etc.)
---@param timeout_ms? integer Timeout in milliseconds (-1 for infinite)
---@return integer|nil triggered_events, string? err
function Socket:wait(events, timeout_ms) end

---Get raw OS file descriptor / socket handle integer.
---@return integer fd
function Socket:get_fd() end

---Retrieve socket family type (`STREAM` or `DGRAM`).
---@return integer socket_type
function Socket:type() end

---Explicitly close socket and free kernel descriptor.
---@return boolean|nil success, string? err
function Socket:close() end

---Create a new uninitialized socket handle.
---@param type integer `cais.socket.type.STREAM` or `cais.socket.type.DGRAM`
---@return CaisSocket|nil sock, string? err
function socket.create(type) end

---Create a connected TCP client socket.
---@param host string Target IP or hostname
---@param port integer Target port
---@param opts? { timeout: integer?, blocking: boolean? }
---@return CaisSocket|nil sock, string? err
function socket.connect(host, port, opts) end

---Create and bind a listening TCP server socket.
---@param host string Bind IP (e.g. "0.0.0.0")
---@param port integer Bind port
---@param backlog? integer Connection backlog queue size (default: 128)
---@return CaisSocket|nil server_sock, string? err
function socket.bind(host, port, backlog) end

---Convenience constructor for uninitialized TCP socket.
---@return CaisSocket|nil sock, string? err
function socket.tcp() end

---Convenience constructor for uninitialized UDP socket.
---@return CaisSocket|nil sock, string? err
function socket.udp() end

---Create and bind a UDP listening socket.
---@param host string Host interface
---@param port integer Port number
---@return CaisSocket|nil sock, string? err
function socket.udp_bind(host, port) end

---Multiplex multiple readable and writable sockets simultaneously.
---@param readers CaisSocket[] Array of sockets to check for readability
---@param writers CaisSocket[] Array of sockets to check for writability
---@param timeout_ms? integer Timeout in milliseconds (-1 for indefinite)
---@return CaisSocket[] ready_readers, CaisSocket[] ready_writers, string? err
function socket.select(readers, writers, timeout_ms) end

return socket
