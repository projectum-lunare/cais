---@meta cais.sandbox

---
---# cais.sandbox
---
---Process isolation and kernel sandboxing (Landlock, Pledge, Capsicum, AppContainer).
---Provides both declarative policy objects and fluent OOP builders.
---
local sandbox = {}

---@class CaisSandboxBackend
---@field NONE integer No kernel sandbox available
---@field PLEDGE integer OpenBSD pledge(2) / unveil(2)
---@field LANDLOCK integer Linux Landlock LSM
---@field CAPSICUM integer FreeBSD Capsicum capability engine
---@field APPCONTAINER integer Windows AppContainer
sandbox.backend = {}

---@class CaisSandboxPerm
---@field READ integer Read-only filesystem access
---@field WRITE integer Write-only filesystem access
---@field EXEC integer Execution permission
---@field READWRITE integer Read and write filesystem access
---@field ALL integer Read, write, and execute permissions
sandbox.perm = {}

---@class CaisSandboxPolicy
local SandboxPolicy = {}

---Allow filesystem access to specified directory or file path.
---@param path string Target directory or file path
---@param perm integer|"read"|"write"|"exec"|"readwrite"|"all" Permission mask
---@return CaisSandboxPolicy self Method chaining support
function SandboxPolicy:allow(path, perm) end

---Configure network access capability for policy.
---@param allow boolean True to allow network sockets, false to restrict
---@return CaisSandboxPolicy self
function SandboxPolicy:set_network(allow) end

---Alias for `set_network()`.
---@param allow boolean True to allow network sockets, false to restrict
---@return CaisSandboxPolicy self
function SandboxPolicy:network(allow) end

---Restrict all network socket creation and I/O.
---@return CaisSandboxPolicy self
function SandboxPolicy:deny_network() end

---Permit or forbid spawning child processes (fork/exec).
---@param allow boolean True to allow child processes, false to restrict
---@return CaisSandboxPolicy self
function SandboxPolicy:set_child_processes(allow) end

---Forbid spawning any child processes or executing new binaries.
---@return CaisSandboxPolicy self
function SandboxPolicy:no_child_processes() end

---Permit or forbid dynamic code execution (JIT / W^X).
---@param allow boolean True to allow dynamic code, false to enforce W^X
---@return CaisSandboxPolicy self
function SandboxPolicy:set_dynamic_code(allow) end

---Prevent creating executable memory mappings (enforce W^X).
---@return CaisSandboxPolicy self
function SandboxPolicy:no_dynamic_code() end

---Enforce this sandbox policy on the current process immediately.
---@return boolean|nil success, string? err
function SandboxPolicy:apply() end

---Retrieve active host kernel sandbox backend identifier.
---@return integer backend `cais.sandbox.backend.LANDLOCK`, etc.
function sandbox.backend() end

---Alias for `backend()`.
---@return integer backend
function sandbox.get_backend() end

---Check whether the host kernel supports an active sandboxing backend.
---@return boolean is_supported
function sandbox.is_supported() end

---Create a new fluent sandbox policy builder.
---@return CaisSandboxPolicy policy
function sandbox.policy() end

---Alias constructor for `policy()`.
---@return CaisSandboxPolicy policy
function sandbox.Policy() end

---Apply a sandbox policy table or policy object to the running process.
---@param policy CaisSandboxPolicy|{ allow?: table<string, integer|string>, network?: boolean, child_processes?: boolean, dynamic_code?: boolean }
---@return boolean|nil success, string? err
function sandbox.apply(policy) end

return sandbox
