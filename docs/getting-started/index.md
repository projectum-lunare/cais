# Quickstart Guide

This guide walks you through integrating `cais` into your project, whether you are writing modern C, ANSI C89 systems code, or building applications in Lua.

---

## 1. Choosing Your Integration Path

`cais` can be consumed in three distinct ways:

1. **C Static Library (`libcais.a`):** For statically linked executables with zero runtime dependencies.
2. **C Dynamic Shared Object (`cais.so` / `cais.dll`):** For modular deployment or standard dynamic linking.
3. **Lua Native Modules (`cais.*`):** Require the full umbrella library or load isolated, ultra-compact submodules directly.

---

## 2. Minimal Example in C

Save as `main.c`:

```c
#include <stdio.h>
#include <cais/cais.h>

int main(void) {
    printf("cais version: %s\n", cais_version());
    printf("Host Operating System: %s\n", cais_platform_os());
    printf("Host Architecture:     %s\n", cais_platform_arch());

    /* Canonical boolean predicate */
    if (cais_platform_is_64bit() == CAIS_TRUE) {
        printf("Environment is 64-bit.\n");
    } else {
        printf("Environment is 32-bit (or 16-bit).\n");
    }

    return 0;
}
```

Compile with standard GCC/Clang:
```bash
gcc -std=c89 -Wall -Wextra -Iinclude main.c -Lbuild/linux/x86_64/release -lcais -o main
./main
```

---

## 3. Minimal Example in Lua

Save as `example.lua`:

```lua
local cais = require("cais")

print("OS: " .. cais.platform.os)
print("Arch: " .. cais.platform.arch)

-- Verify path existence using canonical boolean return
if cais.fs.exists("main.c") then
    print("Source code file is present on disk.")
end
```

Run with standard Lua 5.1..5.4 or LuaJIT:
```bash
lua example.lua
```

---

## Next Steps

- [Building & Compiling](building.md): Build targets, xmake flags, standalone submodules.
- [C89 API Usage](c-api.md): Header layout, error handling conventions, and memory rules.
- [Lua API Usage](lua-api.md): Object-oriented wrappers, options tables, and garbage collection.
