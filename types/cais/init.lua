---@meta cais

---
---# cais (Umbrella Module)
---
---Lightweight, modular C89 and Lua systems programming substrate
---for contemporary workstations and vintage platforms.
---
---Subsystems materialize lazily upon first access.
---
local cais = {}

---Retrieve runtime library version string (e.g. "0.1.0").
---@return string version
function cais.version() end

---Platform and CPU architecture introspection.
---@type cais.platform
cais.platform = require("cais.platform")

---Portable VFS, file streaming, and directory traversal.
---@type cais.filesystem
cais.filesystem = require("cais.filesystem")

---Short alias for `cais.filesystem`.
---@type cais.filesystem
cais.fs = cais.filesystem

---Network sockets, TCP, UDP, and multi-socket select().
---@type cais.socket
cais.socket = require("cais.socket")

---Physical RS-232 / UART serial communication driver.
---@type cais.serial
cais.serial = require("cais.serial")

---IEEE 1284 / Centronics bidirectional parallel port driver.
---@type cais.parallel
cais.parallel = require("cais.parallel")

---Process containment and kernel sandboxing.
---@type cais.sandbox
cais.sandbox = require("cais.sandbox")

---Noise-Based Cryptographic Transport Substrate.
---@type cais.noise
cais.noise = require("cais.noise")

return cais
