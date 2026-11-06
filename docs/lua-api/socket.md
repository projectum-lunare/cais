# cais.socket

The `cais.socket` module provides a comprehensive network substrate supporting TCP streaming connections, UDP datagrams, non-blocking I/O, synchronous timeouts, and multi-socket event multiplexing with `select()`.

---

## Requiring the Module

```lua
local socket = require("cais.socket")
```

---

## Enumeration Tables

### `socket.mode`

* `socket.mode.BLOCKING`: Socket calls block until completion or timeout.
* `socket.mode.NONBLOCKING`: Socket calls return immediately with `CAIS_SOCK_AGAIN` if I/O cannot proceed.

### `socket.type`

* `socket.type.STREAM`: Stream-oriented protocol (TCP).
* `socket.type.DGRAM`: Datagram-oriented protocol (UDP).

### `socket.event`

* `socket.event.READ`: Readiness mask for reading.
* `socket.event.WRITE`: Readiness mask for writing.
* `socket.event.ERROR`: Exception/error mask.

---

## Socket Constructors

### `socket.connect(host, port, opts?)`

Create a connected TCP client socket.

```lua
local sock, err = socket.connect("127.0.0.1", 8080, {
    timeout = 3000, -- 3 second connection/read timeout
    blocking = true
})
```

### `socket.bind(host, port, backlog?)`

Create and bind a TCP listening server socket.

```lua
local server, err = socket.bind("0.0.0.0", 8080, 128)
```

### `socket.tcp()`

Create an uninitialized TCP streaming socket handle.

### `socket.udp()`

Create an uninitialized UDP datagram socket handle.

### `socket.udp_bind(host, port)`

Create and bind a UDP socket to a local interface and port.

---

## `Socket` Userdata Methods

### `sock:send(data)` / `sock:write(data)`

Transmit binary or text data over a connected TCP stream.

* **Returns:** `integer` (bytes transmitted) or `nil, err`.

### `sock:recv(max_bytes?)` / `sock:read(max_bytes?)`

Receive incoming payload from stream.

* **Parameters:** `max_bytes` (integer, optional, defaults to 4096).
* **Returns:** `string` (received bytes) or `nil, err`.

### `sock:sendto(data, host, port)`

Transmit UDP datagram payload to a remote destination.

* **Returns:** `integer` (bytes sent) or `nil, err`.

### `sock:recvfrom(max_bytes?)`

Receive incoming UDP datagram.

* **Returns:** `data, sender_ip, sender_port` or `nil, err`.

```lua
local data, sender_ip, sender_port = udp_sock:recvfrom(2048)
print(string.format("Received %d bytes from %s:%d", #data, sender_ip, sender_port))
```

### `sock:accept()`

Accept an incoming client connection on a listening TCP server socket.

* **Returns:** `client_sock, client_ip, client_port` or `nil, err`.

```lua
local client, ip, port = server:accept()
if client then
    client:send("Welcome to Cais TCP Server\n")
    client:close()
end
```

### `sock:set_timeout(timeout_ms)`

Set synchronous I/O timeout in milliseconds (`-1` for infinite, `0` for immediate non-blocking).

### `sock:set_blocking(blocking)`

Enable (`true`) or disable (`false`) blocking I/O behavior.

### `sock:is_blocking()` / `sock:is_nonblocking()`

Returns `boolean` reflecting current socket operational mode.

### `sock:wait(events, timeout_ms?)`

Poll a single socket for event readiness (`socket.event.READ`, `WRITE`).

* **Returns:** Triggered events bitmask or `nil, err`.

### `sock:get_fd()`

Retrieve raw underlying OS socket descriptor / file handle integer.

### `sock:close()`

Explicitly shut down and close the socket handle.

---

## Multiplexing with `socket.select()`

Multiplex multiple readers and writers simultaneously without thread contention:

```lua
local readers = { client1, client2, server }
local writers = { client1 }

local ready_readers, ready_writers, err = socket.select(readers, writers, 100) -- 100ms timeout

for _, r in ipairs(ready_readers) do
    if r == server then
        local new_client = server:accept()
        table.insert(readers, new_client)
    else
        local data = r:recv(1024)
        if not data or #data == 0 then
            r:close()
        else
            r:send("Echo: " .. data)
        end
    end
end
```
