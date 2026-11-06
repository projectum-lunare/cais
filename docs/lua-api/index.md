# Lua API Reference

`cais` provides idiomatic, high-performance **Lua (>= 5.4)** bindings for all underlying C89 subsystems. It is designed around object-oriented userdata wrappers, multi-return error tuples, and automatic garbage collection.

---

## Subsystem Modules

<div class="grid cards" markdown>

- :material-layers-outline: **[`cais` (Umbrella)](cais.md)**

    ---

    Lazy-loading umbrella module. Loads all subsystems on demand upon first table access.

- :material-chip: **[`cais.platform`](platform.md)**

    ---

    Runtime OS, CPU architecture, word size (16/32/64-bit), and endianness detection.

- :material-folder-multiple-outline: **[`cais.filesystem`](filesystem.md)**

    ---

    Streaming file I/O (`FileHandle`), directory traversal, VFS status, and path operations. Also aliased as `cais.fs`.

- :material-lan: **[`cais.socket`](socket.md)**

    ---

    TCP client/server, UDP datagrams, non-blocking asynchronous multiplexing (`select()`), and timeouts.

- :material-serial-port: **[`cais.serial`](serial.md)**

    ---

    Physical RS-232 / UART serial communication, baud rates, hardware flow control, and modem lines.

- :material-printer-outline: **[`cais.parallel`](parallel.md)**

    ---

    Centronics / IEEE 1284 bidirectional parallel port strobe pulses, status pins, and SPP/EPP/ECP modes.

- :material-shield-lock-outline: **[`cais.sandbox`](sandbox.md)**

    ---

    Kernel security containment via Landlock, Pledge, Capsicum, or AppContainer with fluent OOP policy builders.

- :material-lock-check-outline: **[`cais.noise`](noise.md)**

    ---

    Noise Protocol cryptographic transport (`Noise_[Pattern]_25519_ChaChaPoly_BLAKE2s`) with mutual authentication and 1-RTT sessions.

- :material-security: **[`cais.tls`](tls.md)**

    ---

    Modular Transport Layer Security (TLS) engine with pluggable backends (BearSSL) and custom I/O decoupling; protocol versions depend on the backend driver.

</div>

---

## Core Lua Conventions

### 1. Multi-Return Error Handling

All fallible functions adhere to standard Lua idioms:

- **On Success:** Returns the desired object, count, or `true`.
- **On Failure:** Returns `nil` followed by a descriptive error string.

```lua
local sock, err = cais.socket.connect("127.0.0.1", 8080)
if not sock then
    error("Connection failed: " .. tostring(err))
end
```

### 2. Userdata Handles and Automatic Cleanup

Native resources (`FileHandle`, `Socket`, `SerialPort`, `ParallelPort`) are represented as Lua userdata with metatables:

- **Explicit Closing:** Every handle provides a `:close()` method.
- **Automatic Garbage Collection (`__gc`):** If a handle goes out of scope, Lua's garbage collector safely closes and releases the underlying OS descriptor without leaking resources.
- **To-Be-Closed Variable Support (`<close>`):** Fully compatible with Lua 5.4's `<close>` annotation:

```lua
do
    local handle <close> = cais.fs.open("data.bin", "rb")
    -- handle is guaranteed to close even if an error is raised inside the block
end
```

### 3. Flexible Constructors

Device and connection openers accept both modern options tables and classic positional arguments:

```lua
-- Modern options table
local port = cais.serial.open("/dev/ttyUSB0", {
    baud = cais.serial.baud.B115200,
    flow = "hardware",
    timeout = 500
})

-- Positional arguments
local port = cais.serial.open("/dev/ttyUSB0", 115200, 8, "N", 1, 500)
```

---

## IDE & Editor Autocomplete (LuaLS / EmmyLua)

Type annotations are included in the repository under [`types/cais/`](https://github.com/projectum-lunare/cais/tree/master/types/cais) with a ready-to-use `.luarc.json`. Any modern editor (VS Code with Lua extension, Neovim with `lua_ls`, Helix, Zed) will automatically provide full autocomplete, parameter hints, and type diagnostics out of the box.
