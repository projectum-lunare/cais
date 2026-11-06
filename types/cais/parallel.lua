---@meta cais.parallel

---
---# cais.parallel
---
---IEEE 1284 / Centronics parallel port driver, hardware pin interrogation,
---and bidirectional strobe interaction.
---
local parallel = {}

---@class CaisParallelMode
---@field SPP integer Standard Parallel Port (Centronics output-only)
---@field EPP integer Enhanced Parallel Port (high-speed bidirectional)
---@field ECP integer Extended Capability Port (hardware FIFO & RLE)
parallel.mode = {}

---@class CaisParallelOptions
---@field mode? integer Operating mode (`cais.parallel.mode.SPP`, etc.)

---@class CaisParallelPort
local ParallelPort = {}

---Transmit byte sequence over 8-bit parallel data lines with hardware STROBE# pulse.
---@param data string Bytes to write
---@return integer|nil bytes_written, string? err
function ParallelPort:write(data) end

---Read incoming byte sequence (requires EPP or ECP bidirectional mode).
---@param max_bytes? integer Maximum bytes to read
---@return string|nil data, string? err
function ParallelPort:read(max_bytes) end

---Retrieve raw hardware status register integer byte.
---@return integer status_byte
function ParallelPort:status() end

---Check if peripheral is asserting BUSY line (printer processing, offline, or buffer full).
---@return boolean is_busy
function ParallelPort:is_busy() end

---Check if peripheral acknowledged byte receipt (ACK# pulse).
---@return boolean is_ack
function ParallelPort:is_ack() end

---Check if Paper End (PE) line is asserted (printer out of paper).
---@return boolean is_pe
function ParallelPort:is_pe() end

---Alias for `is_pe()`.
---@return boolean is_paper_out
function ParallelPort:is_paper_out() end

---Check if peripheral is online and selected (SLCT line asserted).
---@return boolean is_online
function ParallelPort:is_online() end

---Alias for `is_online()`.
---@return boolean is_slct
function ParallelPort:is_slct() end

---Check if peripheral reported an error condition (FAULT# line asserted).
---@return boolean is_fault
function ParallelPort:is_fault() end

---Switch port operating mode between SPP, EPP, and ECP.
---@param mode integer `cais.parallel.mode.SPP`, etc.
---@return boolean|nil success, string? err
function ParallelPort:set_mode(mode) end

---Wait until all hardware FIFOs have emptied and line bus has settled.
---@return boolean|nil success, string? err
function ParallelPort:drain() end

---Alias for `drain()`.
---@return boolean|nil success, string? err
function ParallelPort:flush() end

---Explicitly close parallel device handle.
---@return boolean|nil success, string? err
function ParallelPort:close() end

---Open a physical or emulated parallel port device.
---@param path string Parallel device path (e.g. "/dev/parport0", "LPT1")
---@param opts? CaisParallelOptions
---@return CaisParallelPort|nil port, string? err
function parallel.open(path, opts) end

return parallel
