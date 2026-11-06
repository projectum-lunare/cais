# cais.sandbox

The `cais.sandbox` module provides process isolation and kernel containment across supported platforms using native security engines: **Landlock** on Linux, **Pledge/Unveil** on OpenBSD, **Capsicum** on FreeBSD, and **AppContainer** on Windows.

It supports both declarative policy tables and a fluent, chainable object-oriented **Policy Builder**.

---

## Requiring the Module

```lua
local sandbox = require("cais.sandbox")
```

---

## Enumeration Tables

### `sandbox.backend`

* `sandbox.backend.NONE`: No kernel sandboxing engine available on host.
* `sandbox.backend.PLEDGE`: OpenBSD `pledge(2)` and `unveil(2)`.
* `sandbox.backend.LANDLOCK`: Linux Landlock LSM kernel subsystem.
* `sandbox.backend.CAPSICUM`: FreeBSD Capsicum capability mode.
* `sandbox.backend.APPCONTAINER`: Windows AppContainer token isolation.

### `sandbox.perm`

* `sandbox.perm.READ`: Read-only access to path and children.
* `sandbox.perm.WRITE`: Write-only access to path and children.
* `sandbox.perm.EXEC`: Execution permission for binaries and scripts.
* `sandbox.perm.READWRITE`: Read and write access.
* `sandbox.perm.ALL`: Full read, write, and execute permissions.

---

## Module Functions

### `sandbox.is_supported()`

Check whether an active sandboxing engine is present and supported by the running host kernel.

* **Returns:** `boolean`

### `sandbox.backend()` / `sandbox.get_backend()`

Retrieve the active backend identifier constant (`sandbox.backend.LANDLOCK`, etc.).

* **Returns:** `integer`

### `sandbox.apply(policy)`

Apply a sandbox policy immediately to the current process. Can be passed either a declarative table or a `SandboxPolicy` builder object.

```lua
-- Declarative table apply
local ok, err = sandbox.apply({
    allow = {
        ["/var/log/app"] = "readwrite",
        ["/etc/app.conf"] = "read"
    },
    network = false,
    child_processes = false,
    dynamic_code = false
})
```

---

## Object-Oriented Policy Builder

Construct policies with fluent, chainable methods:

```lua
local policy = sandbox.policy()
```

### Chaining Methods

| Method                              | Parameters                            | Description                                                                                                  |
| :---------------------------------- | :------------------------------------ | :----------------------------------------------------------------------------------------------------------- |
| `policy:allow(path, perm)`          | `path: string, perm: string\|integer` | Add directory or file path with specified permission (`"read"`, `"write"`, `"exec"`, `"readwrite"`, `"all"`) |
| `policy:set_network(allow)`         | `allow: boolean`                      | Permit or deny network socket creation and I/O (alias: `policy:network(allow)`)                              |
| `policy:deny_network()`             | —                                     | Convenience method to forbid all networking                                                                  |
| `policy:set_child_processes(allow)` | `allow: boolean`                      | Permit or deny spawning subprocesses (`fork`, `exec`)                                                        |
| `policy:no_child_processes()`       | —                                     | Forbid spawning child processes                                                                              |
| `policy:set_dynamic_code(allow)`    | `allow: boolean`                      | Permit or forbid dynamic code execution / memory mapping (W^X)                                               |
| `policy:no_dynamic_code()`          | —                                     | Enforce strict W^X memory execution prevention                                                               |
| `policy:apply()`                    | —                                     | Lock down the running process using this configured policy                                                   |

---

## Complete OOP Sandbox Example

```lua
local sandbox = require("cais.sandbox")

if not sandbox.is_supported() then
    print("Sandboxing not supported on this kernel; skipping isolation.")
    return
end

-- Build policy with method chaining
local ok, err = sandbox.policy()
    :allow("/usr/lib", "read")
    :allow("/usr/share", "read")
    :allow("/tmp/app_scratch", "readwrite")
    :deny_network()
    :no_child_processes()
    :no_dynamic_code()
    :apply()

if not ok then
    error("Failed to enforce sandbox: " .. tostring(err))
end

print("Process locked down successfully under backend " .. sandbox.backend())
```
