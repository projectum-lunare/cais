# cais.tls

The `cais.tls` module provides an extensible, modular Transport Layer Security (TLS) engine for establishing encrypted and authenticated sessions over stream sockets (`cais.socket`) or arbitrary custom I/O callbacks.

---

## Requiring the Module

Available either directly or through the lazy umbrella module:

```lua
-- Standalone direct require:
local tls = require("cais.tls")

-- Or via umbrella cais:
local cais = require("cais")
local tls = cais.tls
```

---

## Introspection Functions

### `tls.is_supported()`
Returns `true` if a functional cryptographic TLS backend (e.g., BearSSL) is compiled into the binary, or `false` if running on the stub backend.

```lua
if tls.is_supported() then
    print("TLS acceleration available via: " .. tls.backend())
end
```

### `tls.backend()`
Returns the identifier of the active TLS driver engine as a string (e.g. `"bearssl"`, `"wolfssl"`, or `"stub"`).

---

## Enumeration Tables

### `tls.status`
Operation status codes returned by handshake and I/O methods:

| Status Constant | Value | Description |
| :-------------- | :---: | :---------- |
| `tls.status.OK` | `0` | Operation succeeded. |
| `tls.status.ERROR` | `-1` | Operation failed (check error message). |
| `tls.status.WANT_READ` | `1` | Transport needs to read incoming network packets to proceed. |
| `tls.status.WANT_WRITE` | `2` | Engine has pending outbound packets to transmit. |
| `tls.status.CLOSED` | `3` | Remote peer initiated a clean TLS `close_notify` teardown. |

### `tls.role`
Handshake role:

* `tls.role.CLIENT` (`0`): Outbound TLS client.
* `tls.role.SERVER` (`1`): Inbound TLS server.

### `tls.verify`
Certificate verification policy:

* `tls.verify.NONE` (`0`): Insecure mode. Disables certificate validation (useful for embedded devices without root CAs or local testing).
* `tls.verify.PEER` (`1`): Standard mode. Requires valid certificate chain matching trusted CAs.

---

## Session Constructors

### `tls.wrap(socket, [options])`
Wraps an existing connected `cais.socket` handle with a TLS session.

* **`socket`** (`cais.socket`): Connected stream socket.
* **`options`** (`table`, optional):
  * `server_name` (`string`): SNI (Server Name Indication) hostname sent in `ClientHello`.
  * `role` (`integer`): `tls.role.CLIENT` (default) or `tls.role.SERVER`.
  * `verify` (`integer`): `tls.verify.NONE` (default) or `tls.verify.PEER`.

Returns the new `TlsSession` userdata, or `nil, error_message` on failure.

```lua
local sock = cais.socket.connect("example.com", 443)
local session, err = tls.wrap(sock, {
    server_name = "example.com",
    verify = tls.verify.NONE
})
```

### `tls.create([options])`
Creates an unanchored TLS session without an attached socket. Transport I/O must subsequently be bound via `:set_io()`.

```lua
local session, err = tls.create({
    server_name = "myhost.local",
    role = tls.role.CLIENT,
    verify = tls.verify.NONE
})
```

---

## Session Methods

### `session:set_io(read_fn, write_fn)`
Binds custom Lua functions for bidirectional non-blocking record transport.

* **`read_fn(max_bytes)`**: Invoked when the TLS engine needs raw data from the transport. Must return a string containing bytes read, or `nil` on EOF / error.
* **`write_fn(data)`**: Invoked when the TLS engine wants to transmit ciphertext over the transport. Must return the number of bytes written, or `nil` on error.

### `session:handshake()`
Drives the TLS handshake state machine forward.

Returns:
* `tls.status.OK` if the handshake is complete and application data can now be sent.
* `tls.status.WANT_READ` or `tls.status.WANT_WRITE` if asynchronous I/O must complete before handshaking can advance.
* `tls.status.ERROR, err_msg` on negotiation or cryptographic error.

```lua
local status = session:handshake()
while status == tls.status.WANT_READ or status == tls.status.WANT_WRITE do
    -- Drive underlying transport or sleep briefly
    status = session:handshake()
end
```

### `session:send(data)` (or `:write(data)`)
Encrypts and transmits plaintext data across the TLS tunnel.

* **`data`** (`string`): Plaintext bytes to encrypt and send.

Returns:
* `bytes_sent` (`integer`): Number of plaintext bytes accepted by the TLS engine.
* On error or non-blocking hold: `nil, err_msg, status_code`.

### `session:recv([max_bytes])` (or `:read([max_bytes])`)
Decrypts and receives application data from the TLS tunnel.

* **`max_bytes`** (`integer`, default `4096`): Maximum plaintext buffer size.

Returns:
* `data` (`string`): Decrypted plaintext payload.
* On EOF / closure: `""` (empty string).
* On hold / pending read: `nil, err_msg, status_code`.

### `session:close()`
Sends a TLS `close_notify` alert to cleanly terminate the cryptographic session.

---

## Complete Example: Fetching HTTPS Webpage

```lua
local cais = require("cais")
local tls = cais.tls

-- 1. Connect TCP socket
local sock, err = cais.socket.connect("www.example.com", 443, { blocking = true })
assert(sock, err)

-- 2. Wrap socket in TLS session
local ssl, err = tls.wrap(sock, {
    server_name = "www.example.com",
    verify = tls.verify.NONE
})
assert(ssl, err)

-- 3. Perform TLS Handshake
local status = ssl:handshake()
while status == tls.status.WANT_READ or status == tls.status.WANT_WRITE do
    status = ssl:handshake()
end
assert(status == tls.status.OK, "Handshake failed")

-- 4. Send HTTP GET Request
ssl:send("GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: close\r\n\r\n")

-- 5. Read response
local resp = {}
while true do
    local chunk, err, code = ssl:recv(4096)
    if not chunk or #chunk == 0 then break end
    table.insert(resp, chunk)
end

print("Response:\n" .. table.concat(resp))

-- 6. Cleanup
ssl:close()
sock:close()
```
