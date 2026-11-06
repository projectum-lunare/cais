# Kernel Sandboxing (`cais.sandbox`)

The `sandbox` subsystem provides operating system-enforced application containment using native kernel security primitives across **Linux**, **macOS Darwin**, **Windows NT**, **OpenBSD**, and **FreeBSD**.

Instead of forcing developers to write platform-specific code for every operating system, `cais` introduces a unified **declarative policy engine**: applications declare their access requirements (allowed filesystem paths, network permissions, child process policies, and memory execution rules), and `cais` automatically translates these declarations into the native kernel mechanism of the host system.

---

## 🏛️ 1. Supported Backends & Feature Matrix

Operating systems vary dramatically in how they implement process isolation. The table below illustrates how `cais` maps declarative policy fields across all supported backends:

| Backend Identifier              | Name String  | Native Kernel Mechanism           | Filesystem Scoping |  Network Restrictions  | Child Process Prohibition | Dynamic Code Hardening (W^X) | Minimum OS Version           |
| :------------------------------ | :----------- | :-------------------------------- | :----------------: | :--------------------: | :-----------------------: | :--------------------------: | :--------------------------- |
| `CAIS_SANDBOX_BACKEND_LANDLOCK` | `"landlock"` | Linux Landlock LSM + `prctl`      |     ✅ ABIs 1–5     | ✅ ABI 4+ (Linux 6.7+)  |   ✅ Via `no_new_privs`    |       ⚠️ Safe fallback        | **Linux 5.13+**              |
| `CAIS_SANDBOX_BACKEND_SEATBELT` | `"seatbelt"` | Apple Seatbelt / MAC Framework    | ✅ TinyScheme SBPL  |  ✅ `(deny network*)`   |  ✅ `(deny process-fork)`  |       ⚠️ Via macOS SIP        | **Mac OS X 10.5+** (Leopard) |
| `CAIS_SANDBOX_BACKEND_WIN32`    | `"win32"`    | Process Mitigation Policies       | ⚠️ Via ACLs/Tokens  |   ⚠️ Windows Firewall   |     ✅ Win 10 (1607+)      |     ✅ Win 8.1+ / Win 10      | **Windows 8.1 / 10+**        |
| `CAIS_SANDBOX_BACKEND_OPENBSD`  | `"openbsd"`  | OpenBSD `unveil(2)` & `pledge(2)` |  ✅ `unveil` paths  |   ✅ `pledge` `inet`    |   ✅ Drops `proc`/`exec`   |     ✅ W^X enforced by OS     | **OpenBSD 6.4+**             |
| `CAIS_SANDBOX_BACKEND_CAPSICUM` | `"capsicum"` | FreeBSD Capsicum Capability Mode  | ✅ Capability Mode  | ✅ File Descriptor Mode |      ✅ Drops `fork`       |        ⚠️ Host default        | **FreeBSD 10.0+**            |
| `CAIS_SANDBOX_BACKEND_NONE`     | `"none"`     | Safe fallback stub                |      ⚠️ No-op       |        ⚠️ No-op         |          ⚠️ No-op          |           ⚠️ No-op            | All other platforms          |

> [!IMPORTANT]
> **The Ratchet Principle (Irreversible Enforcement):**
> Kernel sandboxing is strictly **unidirectional**. Once a policy is applied via `cais_sandbox_apply()` or Lua `sandbox.apply()`, privileges can **never** be regained or expanded for the lifetime of that process, and all restrictions are automatically inherited by any child processes.

---

## ⚙️ 2. Constants & Enumerations

### Backend Identifiers (`cais_sandbox_backend_t`)

Returned by `cais_sandbox_backend()` in C or `cais.sandbox.backend()` in Lua:

```c
typedef enum {
    CAIS_SANDBOX_BACKEND_NONE     = 0, /* No kernel sandbox available on host */
    CAIS_SANDBOX_BACKEND_LANDLOCK = 1, /* Linux Landlock LSM */
    CAIS_SANDBOX_BACKEND_OPENBSD  = 2, /* OpenBSD pledge(2) and unveil(2) */
    CAIS_SANDBOX_BACKEND_CAPSICUM = 3, /* FreeBSD Capsicum capability mode */
    CAIS_SANDBOX_BACKEND_SEATBELT = 4, /* Apple Darwin / macOS Seatbelt (sandbox_init) */
    CAIS_SANDBOX_BACKEND_WIN32    = 5  /* Windows NT Process Mitigation Policies */
} cais_sandbox_backend_t;
```

In Lua, these constants are accessible in the `cais.sandbox.backend` table:
```lua
local sandbox = require("cais.sandbox")
print(sandbox.backend.LANDLOCK) -- 1
print(sandbox.backend.SEATBELT) -- 4
print(sandbox.backend.WIN32)    -- 5
```

---

### Filesystem Permission Flags (`cais_sandbox_perm_t`)

Bitmasks determining what operations are allowed on registered paths:

| C Constant                | Lua String Equivalent |   Bitmask Value   | Description                                                      |
| :------------------------ | :-------------------: | :---------------: | :--------------------------------------------------------------- |
| `CAIS_SANDBOX_PERM_NONE`  |         `""`          |        `0`        | No access permitted                                              |
| `CAIS_SANDBOX_PERM_READ`  |         `"r"`         | `1 << 0` (`0x01`) | Read file contents, inspect directory entries, read symlinks     |
| `CAIS_SANDBOX_PERM_WRITE` |         `"w"`         | `1 << 1` (`0x02`) | Create, modify, overwrite, truncate, or delete files/directories |
| `CAIS_SANDBOX_PERM_EXEC`  |         `"x"`         | `1 << 2` (`0x04`) | Execute binaries or scripts located in the specified directory   |

Convenience macro aliases in C: `CAIS_SANDBOX_READ`, `CAIS_SANDBOX_WRITE`, `CAIS_SANDBOX_EXEC`.

In Lua, combine permissions as strings: `"r"`, `"w"`, `"rw"`, `"rx"`, or `"rwx"`.

---

### Network Access Policy Levels (`cais_sandbox_net_t`)

Controls network socket capabilities:

| C Constant                  |  Lua String  | Value | Description                                                      |
| :-------------------------- | :----------: | :---: | :--------------------------------------------------------------- |
| `CAIS_SANDBOX_NET_DEFAULT`  | `"default"`  |  `0`  | Default system behavior (unrestricted)                           |
| `CAIS_SANDBOX_NET_NONE`     |   `"none"`   |  `1`  | Completely block network sockets and communication               |
| `CAIS_SANDBOX_NET_LOOPBACK` | `"loopback"` |  `2`  | Allow only local loopback traffic (`127.0.0.1` / `::1`)          |
| `CAIS_SANDBOX_NET_ALL`      |   `"all"`    |  `3`  | Explicitly permit all outbound and inbound network communication |

---

### Status Codes (`cais_sandbox_status_t`)

| C Constant                 | Lua Enum (`sandbox.status`)  | Value | Description                                      |
| :------------------------- | :--------------------------- | :---: | :----------------------------------------------- |
| `CAIS_SANDBOX_OK`          | `sandbox.status.OK`          |  `0`  | Policy enforced successfully                     |
| `CAIS_SANDBOX_ERROR`       | `sandbox.status.ERROR`       | `-1`  | Kernel error, invalid path, or permission denied |
| `CAIS_SANDBOX_UNSUPPORTED` | `sandbox.status.UNSUPPORTED` | `-4`  | Platform lacks a functional sandbox backend      |

---

## 🛠️ 3. C89 Function Reference

All C APIs conform strictly to ANSI C89 (C90) and are declared in [`include/cais/sandbox.h`](file:///home/gustavo/Projetos/cais/include/cais/sandbox.h).

### Introspection Functions

```c
cais_sandbox_backend_t cais_sandbox_backend(int *version_out);
const char *cais_sandbox_backend_name(int *version_out);
cais_bool_t cais_sandbox_is_supported(void);
```

* **`cais_sandbox_backend()`:** Returns the active backend identifier enum and optionally writes the kernel ABI version (e.g. Landlock ABI `5`, or Darwin version `24`) to `version_out`.
* **`cais_sandbox_backend_name()`:** Returns a static descriptive string: `"landlock"`, `"seatbelt"`, `"win32"`, `"openbsd"`, `"capsicum"`, or `"none"`.
* **`cais_sandbox_is_supported()`:** Returns `CAIS_TRUE` if a kernel-enforced sandboxing backend is available and active on the host OS.

---

### Policy Construction & Configuration

```c
cais_sandbox_policy_t *cais_sandbox_policy_create(void);
void cais_sandbox_policy_destroy(cais_sandbox_policy_t *policy);

cais_status_t cais_sandbox_policy_allow_path(
    cais_sandbox_policy_t *policy, 
    const char *path, 
    int perms
);

cais_status_t cais_sandbox_policy_set_network(
    cais_sandbox_policy_t *policy, 
    cais_sandbox_net_t net
);

cais_status_t cais_sandbox_policy_set_allow_child_processes(
    cais_sandbox_policy_t *policy, 
    cais_bool_t allow
);

cais_status_t cais_sandbox_policy_set_allow_dynamic_code(
    cais_sandbox_policy_t *policy, 
    cais_bool_t allow
);
```

* **`cais_sandbox_policy_create()`:** Allocates a new empty declarative policy structure. Returns `NULL` on out-of-memory.
* **`cais_sandbox_policy_destroy()`:** Safely frees all memory and path rules associated with the policy.
* **`cais_sandbox_policy_allow_path()`:** Adds a path rule with bitwise permission flags (`CAIS_SANDBOX_PERM_READ | CAIS_SANDBOX_PERM_WRITE`).
* **`cais_sandbox_policy_set_network()`:** Configures network access policy.
* **`cais_sandbox_policy_set_allow_child_processes()`:** Passing `CAIS_FALSE` blocks `fork`, `exec`, and `CreateProcess`.
* **`cais_sandbox_policy_set_allow_dynamic_code()`:** Passing `CAIS_FALSE` enforces W^X memory protections (no executable heap/stack).

---

### Policy Enforcement

```c
cais_sandbox_status_t cais_sandbox_apply(
    const cais_sandbox_policy_t *policy, 
    char *err_buf, 
    size_t err_cap
);
```

Irrevocably locks down the current process and all future descendants according to the policy.

* If `policy == NULL`, applies an **absolute lockdown**: drops all filesystem access and drops network/process privileges where supported.
* If the call fails, a human-readable diagnostic message is copied into `err_buf`.
* Returns `CAIS_SANDBOX_OK` (`0`), `CAIS_SANDBOX_ERROR` (`-1`), or `CAIS_SANDBOX_UNSUPPORTED` (`-4`).

---

## 💻 4. C89 Programming Examples

### Example 1: Hardened CLI File Processor
This example demonstrates a command-line tool that parses an input directory, writes results to an output directory, and locks down the process so that it cannot touch `/etc`, `C:\Windows`, or access the network:

```c
#include <cais/sandbox.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    char err_buf[256];
    int abi_ver = 0;
    cais_sandbox_policy_t *policy;
    cais_sandbox_status_t status;

    printf("Sandbox backend: %s\n", cais_sandbox_backend_name(&abi_ver));

    /* 1. Allocate declarative policy */
    policy = cais_sandbox_policy_create();
    if (!policy) {
        fprintf(stderr, "Failed to allocate sandbox policy\n");
        return 1;
    }

    /* 2. Configure access rules */
    /* Allow reading from current working directory */
    cais_sandbox_policy_allow_path(policy, ".", CAIS_SANDBOX_PERM_READ);

    /* Allow reading and writing temporary scratch space */
    cais_sandbox_policy_allow_path(policy, "/tmp", CAIS_SANDBOX_PERM_READ | CAIS_SANDBOX_PERM_WRITE);

    /* Strictly block outbound network operations */
    cais_sandbox_policy_set_network(policy, CAIS_SANDBOX_NET_NONE);

    /* Forbid spawning subprocesses */
    cais_sandbox_policy_set_allow_child_processes(policy, CAIS_FALSE);

    /* Forbid creating executable memory (W^X) */
    cais_sandbox_policy_set_allow_dynamic_code(policy, CAIS_FALSE);

    /* 3. Apply and seal the sandbox */
    status = cais_sandbox_apply(policy, err_buf, sizeof(err_buf));
    
    /* Policy is no longer needed after enforcement */
    cais_sandbox_policy_destroy(policy);

    if (status == CAIS_SANDBOX_UNSUPPORTED) {
        printf("Notice: Kernel sandbox not supported on this OS. Continuing...\n");
    } else if (status != CAIS_SANDBOX_OK) {
        fprintf(stderr, "Fatal sandbox error: %s\n", err_buf);
        return 1;
    }

    printf("Process successfully confined! Performing operations safely...\n");

    /* At this point, opening any file outside "." and "/tmp" will be denied by the kernel */
    return 0;
}
```

---

### Example 2: Absolute Lockdown Network Daemon
When building network services, initialize your listening sockets and load your configuration files *first*, then enforce complete lockdown before accepting untrusted client connections:

```c
#include <cais/sandbox.h>
#include <stdio.h>

void enter_jail(void) {
    char err_buf[256];
    
    /* Passing NULL enforces total lockdown:
     * - Linux Landlock: No paths allowed, prctl(PR_SET_NO_NEW_PRIVS)
     * - macOS: TinyScheme profile denying all file operations
     * - Windows: Prohibits dynamic code and subprocess creation
     */
    if (cais_sandbox_apply(NULL, err_buf, sizeof(err_buf)) == CAIS_SANDBOX_ERROR) {
        fprintf(stderr, "Security lockdown failed: %s\n", err_buf);
        exit(1);
    }
}
```

---

## 🌙 5. Lua API Reference & Idioms

In Lua, `cais.sandbox` provides two complementary programming styles:
1. **The Object-Oriented Builder Pattern** (with fluent method chaining).
2. **The Declarative Table DSL** (ideal for configuration-driven applications).

---

### 5.1 Object-Oriented Policy Builder

Created via `cais.sandbox.policy([opts])`. Returns a policy userdata instance.

#### Builder Methods:

* **`policy:allow(path, perms)`**
  Adds a path access rule. `perms` can be `"r"`, `"w"`, `"x"`, `"rw"`, `"rx"`, or `"rwx"`. Returns `self` for chaining.

* **`policy:set_network(level)`**
  Sets the network policy level. Accepts `"none"`, `"loopback"`, `"all"`, or `"default"`. Returns `self`.

* **`policy:deny_network()`**
  Convenience shortcut equivalent to `:set_network("none")`. Returns `self`.

* **`policy:set_child_processes(allow_boolean)`**
  Allows or prohibits spawning child processes (`fork`, `exec`, `CreateProcess`). Returns `self`.

* **`policy:no_child_processes()`**
  Convenience shortcut equivalent to `:set_child_processes(false)`. Returns `self`.

* **`policy:set_dynamic_code(allow_boolean)`**
  Allows or prohibits W^X dynamic code memory allocation. Returns `self`.

* **`policy:no_dynamic_code()`**
  Convenience shortcut equivalent to `:set_dynamic_code(false)`. Returns `self`.

* **`policy:apply()`**
  Permanently enforces the policy on the current process. Returns `true` on success, or `false, err_message, err_code` on failure.

* **`tostring(policy)`**
  Returns a human-readable diagnostic summary (e.g. `<cais.sandbox.policy (paths=2, net=none, child_proc=false, dyn_code=false)>`).

#### OOP Builder Example:
```lua
local sandbox = require("cais.sandbox")

-- Method chaining configuration:
local ok, err = sandbox.policy()
    :allow(".", "r")
    :allow("/tmp", "rw")
    :deny_network()
    :no_child_processes()
    :no_dynamic_code()
    :apply()

if not ok then
    error("Sandbox enforcement failed: " .. tostring(err))
end

print("Sandbox successfully sealed!")
```

---

### 5.2 Declarative Table DSL

Pass a configuration table directly to `cais.sandbox.apply(table)`:

```lua
local sandbox = require("cais.sandbox")

local ok, err, code = sandbox.apply({
    paths = {
        { path = "/var/log/myapp", perm = "rw" },
        { path = "/etc/myapp",     perm = "r"  },
    },
    network = "loopback",        -- "none" | "loopback" | "all" | "default"
    child_processes = false,     -- block fork/exec/CreateProcess
    dynamic_code = false,        -- block RWX memory allocation (W^X)
})

if not ok then
    if code == sandbox.status.UNSUPPORTED then
        print("Warning: Sandbox unsupported on this host OS.")
    else
        error("Security enforcement failed: " .. err)
    end
end
```

---

### 5.3 Polymorphic `cais.sandbox.apply()`

The `apply()` function accepts three distinct input types:

1. **`cais.sandbox.apply(policy_userdata)`**: Applies an OOP builder instance.
2. **`cais.sandbox.apply(table)`**: Compiles and applies a declarative table DSL.
3. **`cais.sandbox.apply()` / `cais.sandbox.apply(nil)`**: Enforces an immediate, zero-trust total lockdown.

---

## 🔬 6. Under the Hood: Native Platform Translation

Understanding how `cais` maps your rules to the host kernel:

### 1. Linux (Landlock LSM)
* **Rule Creation:** Invokes `sys_landlock_create_ruleset()` configuring filesystem access masks:
  * Reads: `LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR`
  * Writes: `LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_MAKE_REG | LANDLOCK_ACCESS_FS_REMOVE_FILE ...`
  * ABI 4+ Network: `LANDLOCK_ACCESS_NET_BIND_TCP | LANDLOCK_ACCESS_NET_CONNECT_TCP`
* **Path Addition:** Opens each path with `O_PATH | O_CLOEXEC` and calls `sys_landlock_add_rule(ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, ...)`.
* **Privilege Dropping:** Calls `prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)`.
* **Enforcement:** Enters containment via `sys_landlock_restrict_self(ruleset_fd, 0)`.

### 2. Apple macOS Darwin (Seatbelt)
* **Dynamic TinyScheme Synthesis:** Rather than static Xcode entitlements, `cais` formats a Sandbox Profile Language (SBPL) Scheme script in memory:
  ```scheme
  (version 1)
  (deny default)
  (allow file-read-metadata)
  (allow sysctl-read)
  (allow mach-lookup)
  (allow signal (target self))
  (deny network*)
  (deny process-fork)
  (allow file-read* (subpath "/path/to/read"))
  (allow file-read* file-write* (subpath "/tmp"))
  ```
* **Enforcement:** Calls `sandbox_init(sbpl_script, 0, &err_msg)` from `libSystem`.

### 3. Windows NT (Process Mitigation Policies)
* **Dynamic Resolution:** Probes `SetProcessMitigationPolicy` dynamically via `GetProcAddress` on `kernel32.dll`. (Preserves compatibility with Windows 7 and XP without loader failure).
* **Child Processes:** Enforces `ProcessChildProcessPolicy` (`CreateProcess` returns `ERROR_ACCESS_DENIED`).
* **Dynamic Code:** Enforces `ProcessDynamicCodePolicy` (prohibits `PAGE_EXECUTE_READWRITE`).
* **Handle Checking:** Enforces `ProcessStrictHandleCheckPolicy`.

### 4. OpenBSD (`unveil` & `pledge`)
* **Filesystem:** Iterates declared paths and calls `unveil(path, perms)`. Calls `unveil(NULL, NULL)` to permanently seal path discovery.
* **Syscall Promises:** Evaluates network and execution flags to synthesize the `pledge()` promise string (e.g. `"stdio rpath wpath cpath inet"`).

### 5. FreeBSD (Capsicum)
* **Capability Mode:** Enters strict capability mode via `cap_enter(2)`. Only pre-opened file descriptors can be accessed.

---

## ⚠️ 7. Production Best Practices & Pitfalls

### 1. The Initialization Order Principle
> [!WARNING]
> Always load Lua modules (`require`), open necessary database handles, read system certificates, and inspect configuration files **before** sealing the sandbox!
> Once the sandbox is applied, attempts to load dynamic `.so` or `.dll` libraries from `/usr/lib` or `C:\Windows` will be intercepted and blocked by the kernel.

```lua
-- CORRECT:
local json = require("cjson")
local socket = require("cais.socket")
local config = load_config("config.json")

-- Seal sandbox AFTER initialization
cais.sandbox.apply({ paths = { { path = "/var/data", perm = "rw" } } })
```

### 2. Testing Sandboxes in Child Processes
Because sandboxing is permanent and cannot be undone within a running process, **never apply a sandbox directly inside a global test runner**:
* In C: Test sandboxes inside a `fork()` block, checking the child's exit status with `waitpid()`.
* In Lua: Execute test scenarios via `os.execute("lua -e '...'")` in an isolated subprocess.

### 3. LuaJIT and Dynamic Code Hardening
The `dynamic_code = false` setting prohibits memory pages with Write+Execute (`PAGE_EXECUTE_READWRITE`). 
* If running on **LuaJIT**, disabling dynamic code will prevent the JIT compiler from allocating machine code stubs. Either disable the JIT first (`jit.off()`) or keep `dynamic_code = true`.
* Standard PUC Lua (5.4 and 5.5) uses a pure bytecode interpreter on the C stack and is **100% compatible** with `dynamic_code = false`.

### 4. Path Canonicalization
When passing paths to `allow()`, prefer canonical absolute paths or consistent relative paths. If a declared path does not exist on disk at the moment `apply()` is invoked, kernel backends like Landlock and Seatbelt will reject the rule with an `ENOENT` error.

---

## 🔍 8. Troubleshooting & Diagnostics

| Symptom                                                      | Probable Cause                                                         | Recommended Resolution                                                               |
| :----------------------------------------------------------- | :--------------------------------------------------------------------- | :----------------------------------------------------------------------------------- |
| `apply()` fails with *"No such file or directory"*           | A path passed to `allow_path` does not exist on disk at apply time.    | Verify path existence with `cais.fs.exists()` before calling `apply()`.              |
| `apply()` returns `CAIS_SANDBOX_UNSUPPORTED` (`-4`)          | Host OS is older than minimum kernel version or sandbox is uncompiled. | Treat `-4` as a non-fatal warning on vintage platforms if full security is optional. |
| Subsequent `require("foo")` fails with *"permission denied"* | Module was required after the sandbox restricted the filesystem.       | Move all `require()` statements to the top of the entry point prior to `apply()`.    |
| Process crashes immediately on Windows                       | Invalid handle passed with Strict Handle Check active.                 | Ensure all file/socket handles are properly checked before closing.                  |
