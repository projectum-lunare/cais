# Recipe: Defense-in-Depth CLI Sandboxing

Command-line utilities processing untrusted user input (such as parsers, converters, or image processors) should enforce the principle of least privilege inside the operating system kernel.

This recipe demonstrates how to:
1. Parse arguments and read local configuration files.
2. Define a declarative sandbox policy (`paths`, `network`, `child_processes`).
3. Apply the policy (`cais_sandbox_apply`) so that any attempted access to `/etc/shadow`, user home directories, or parent folders triggers an immediate permission fault (`EACCES`).

---

## C89 Implementation

```c
#include <stdio.h>
#include <stdlib.h>
#include <cais/cais.h>

int main(int argc, char **argv) {
    char err[128];
    const char *work_dir = ".";
    const char *temp_dir = "/tmp";
    char *config_data = NULL;
    size_t config_len = 0;
    cais_sandbox_policy_t *policy = NULL;

    printf("Starting secure CLI utility...\n");

    /* Step 1: Read trusted configuration before locking down */
    config_data = cais_fs_read_file("config.ini", &config_len);
    if (config_data) {
        printf("Loaded config (%lu bytes)\n", (unsigned long)config_len);
        free(config_data);
    }

    /* Step 2: Configure Sandbox Policy */
    policy = cais_sandbox_policy_create();
    if (policy) {
        /* Read-only on the working directory */
        cais_sandbox_policy_allow_path(policy, work_dir, CAIS_SANDBOX_READ);
        
        /* Read-write on /tmp for temporary scratch output */
        cais_sandbox_policy_allow_path(policy, temp_dir, CAIS_SANDBOX_READ | CAIS_SANDBOX_WRITE);

        /* Prohibit network operations and subprocess creation */
        cais_sandbox_policy_set_network(policy, CAIS_SANDBOX_NET_NONE);
        cais_sandbox_policy_set_allow_child_processes(policy, CAIS_FALSE);

        /* Step 3: Enforce the sandbox permanently */
        if (cais_sandbox_apply(policy, err, sizeof(err)) == CAIS_SANDBOX_OK) {
            printf("Sandbox applied. Kernel restrictions active.\n");
        } else {
            printf("Sandboxing not supported on this OS: %s\n", err);
        }
        cais_sandbox_policy_destroy(policy);
    }

    /* Step 4: Normal operation within bounds */
    cais_fs_write_file("/tmp/output.txt", "Processed data\n", 15);
    printf("Wrote output to /tmp/output.txt successfully.\n");

    /* Step 5: Proof of Security - Unauthorized access fails */
    if (cais_fs_exists("/etc/passwd") == CAIS_FALSE) {
        printf("[CONFIRMED] Host kernel blocked access to /etc/passwd.\n");
    }

    return 0;
}
```

---

## Lua Implementation

```lua
local cais = require("cais")
local fs = cais.fs
local sandbox = cais.sandbox

print("Backend: " .. tostring(sandbox.backend()))

-- 1. Read configuration
if fs.exists("config.json") then
    local conf = fs.read_file("config.json")
    print("Configuration loaded")
end

-- 2. Restrict process privileges via declarative policy
local ok, err = sandbox.apply({
    paths = {
        { path = ".", perm = "r" },      -- Read-only in workspace
        { path = "/tmp", perm = "rw" },  -- Read-write in /tmp
    },
    network = "none",
    child_processes = false,
})

if not ok then
    print("Sandbox warning: " .. tostring(err))
end

-- 3. Permitted operation
fs.write_file("/tmp/result.txt", "OK\n")
print("Saved /tmp/result.txt")

-- 4. Prohibited operation raises error
local ok, err = pcall(function()
    fs.write_file("unauthorized.txt", "FAIL\n")
end)

if not ok then
    print("Security policy enforced by kernel: " .. tostring(err))
end
```
