# cais (Umbrella Module)

The `cais` root module acts as a centralized entry point that exposes all subsystems under a single namespace while employing **deferred initialization** (the Lua equivalent of dead-code elimination).

---

## Requiring the Module

```lua
local cais = require("cais")
```

When required, `cais` does not eagerly initialize all subsystems. Instead, it registers loaders in `package.preload` and only materializes a submodule table when that field is indexed for the first time.

---

## Functions

### `cais.version()`

Retrieve the runtime library semantic version string.

```lua
local ver = cais.version()
print("Cais version: " .. ver) -- e.g. "0.1.0"
```

* **Returns:**
  * `string`: Semantic version identifier.

---

## Subsystem Fields

All submodules are accessible directly as fields on the umbrella table:

| Field             | Module                             | Description                                                         |
| :---------------- | :--------------------------------- | :------------------------------------------------------------------ |
| `cais.platform`   | [`cais.platform`](platform.md)     | OS, CPU architecture, word size, and endianness detection           |
| `cais.filesystem` | [`cais.filesystem`](filesystem.md) | Virtual filesystem, file streaming, and directory iteration         |
| `cais.fs`         | [`cais.filesystem`](filesystem.md) | Short alias for `cais.filesystem` (shares identical table identity) |
| `cais.socket`     | [`cais.socket`](socket.md)         | TCP/UDP networking, non-blocking I/O, and `select()` multiplexing   |
| `cais.serial`     | [`cais.serial`](serial.md)         | RS-232 / UART serial communication driver                           |
| `cais.parallel`   | [`cais.parallel`](parallel.md)     | Centronics / IEEE 1284 parallel port driver                         |
| `cais.sandbox`    | [`cais.sandbox`](sandbox.md)       | Kernel sandboxing and process containment policies                  |

---

## Standalone Submodule Loading

If your project only needs a single subsystem (such as networking or filesystem), you can require submodules individually without loading the umbrella table:

```lua
local fs     = require("cais.fs")
local socket = require("cais.socket")
```
