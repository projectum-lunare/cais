# OpenBSD (Pledge & Unveil)

OpenBSD is widely recognized for its security-first architecture, proactive auditing, strict W^X memory enforcement, and kernel security primitives. `cais` integrates seamlessly with OpenBSD's native security model.

---

## 🏛️ OpenBSD Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All OpenBSD Releases)

The following core subsystems are **universally available** across all OpenBSD releases:

* **`cais.platform`:** Architecture identification (amd64, arm64, sparc64, riscv64, i386) and word size.
* **`cais.filesystem`:** Full POSIX.1-2001 directory traversal (`opendir`/`readdir`), metadata inspection (`stat`), and file streaming.
* **`cais.parallel`:** Standard line printer devices (`/dev/lpt0`..`/dev/lpt2`).
* **`cais.serial`:** Standard POSIX `termios` serial communications (`/dev/cua*`, `/dev/tty*`).
* **`cais.socket`:** Native BSD Sockets with `poll(2)` multiplexing.

---

### 🐡 Sandboxing Support by OpenBSD Generation

#### 🔹 Modern OpenBSD (OpenBSD 6.4 .. 7.6+)

* **Period:** 2018..2026
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Full `unveil(2)` and `pledge(2)` integration:
  * Filesystem paths dynamically applied via `unveil(path, perms)` and sealed with `unveil(NULL, NULL)`.
  * Sycall permissions dynamically synthesized into `pledge()` promise strings (`stdio rpath wpath cpath inet`).
  * Process fork/exec dropped when `allow_child_processes = false`.

#### 🔹 Early Pledge OpenBSD (OpenBSD 5.9 .. 6.3)

* **Period:** 2016..2018
* **Kernel Sandboxing (`cais.sandbox`):** 🟡 Sycall filtering via `pledge(2)` only:
  * Sycall restrictions enforced via kernel promises.
  * Filesystem path scoping (`unveil`) was introduced in 6.4 and is omitted cleanly without errors.

#### 🔹 Vintage OpenBSD (OpenBSD 2.0 .. 5.8)

* **Period:** 1996..2015
* **Kernel Sandboxing (`cais.sandbox`):** ⚠️ Safe fallback stub (returns `CAIS_SANDBOX_UNSUPPORTED` / `-4`). Syscall sandboxing was not yet present in the kernel.

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Kernel-Level Sandboxing (`cais.sandbox`)

OpenBSD provides two orthogonal, complementary kernel security syscalls: **`unveil(2)`** and **`pledge(2)`**. `cais` compiles high-level declarative policies into both mechanisms automatically:

#### Automatic `unveil(2)` Path Confinement

1. For each path in the policy, `cais` translates permissions:
   * `CAIS_SANDBOX_READ` → `"r"`
   * `CAIS_SANDBOX_WRITE` → `"wc"` (write and create)
   * `CAIS_SANDBOX_EXEC` → `"x"`
2. Calls `unveil(path, mode)` for each configured directory and file.
3. Once all paths are registered, calls `unveil(NULL, NULL)` to **permanently seal** the unveil table, preventing any subsequent path openings across the remainder of the filesystem.

#### Automatic `pledge(2)` Promise Synthesis

`cais` analyzes the policy and dynamically builds the optimal promise string:
* Always includes `"stdio"` (basic I/O and descriptor memory operations).
* Adds `"rpath"` if any path grants read permissions.
* Adds `"wpath cpath"` if any path grants write permissions.
* Adds `"exec"` if any path grants execution permissions.
* Adds `"inet"` if `network != "none"`.
* Adds `"proc"` if `child_processes != false`.
* Calls `pledge(promises, NULL)`.

The developer never needs to memorize OpenBSD pledge strings; declaring capabilities in `cais` configures both `unveil` and `pledge` simultaneously.

### 2. Networking & Socket Multiplexing (`cais.socket`)

* **Pledge Integration:** When network operations are permitted, TCP/UDP sockets operate under the `"inet"` promise.
* **`poll(2)` Multiplexing:** Native linear array multiplexing with stack-allocated scratch buffers for zero-allocation performance.

### 3. Industrial Serial UART (`cais.serial`)

* **Device Nodes:** Uses standard OpenBSD dial-out devices (`/dev/cua00`, `/dev/cua01`, `/dev/cuaU0` for USB).
* **POSIX `termios`:** Baud rates, parity control, and modem line monitoring.

### 4. Parallel Port (`cais.parallel`)

* **Line Printer Driver:** Direct hardware I/O over OpenBSD parallel printer nodes (`/dev/lpt0`, `/dev/lpt1`).

---

## ⏱️ Version Milestones

| Feature                          | Minimum OpenBSD Version | Description                              |
| :------------------------------- | :---------------------: | :--------------------------------------- |
| **Core Library & POSIX I/O**     |   OpenBSD 2.0+ (1996)   | Standard ANSI C89 and POSIX interfaces   |
| **`pledge(2)` Syscall Lockdown** |   OpenBSD 5.9+ (2016)   | Process-level syscall restriction        |
| **`unveil(2)` Path Confinement** |   OpenBSD 6.4+ (2018)   | Kernel-level filesystem path restriction |
| **Simultaneous Pledge + Unveil** |   OpenBSD 6.4+ (2018)   | Full declarative sandboxing in `cais`    |
