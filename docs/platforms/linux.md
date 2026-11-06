# Linux (Modern, Embedded & Real-Time)

`cais` provides first-class support for the Linux kernel across all architectures (x86_64, i386, aarch64, armhf, riscv64, mips, ppc) and C standard libraries (**glibc**, **musl**, **uClibc-ng**, **bionic**).

---

## 🏛️ Linux Kernel Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All Linux Distributions)

The following core subsystems are **universally available** across all Linux distributions:

* **`cais.platform`:** Architecture identification (x86_64, aarch64, riscv64, armhf, mips, ppc), word size, and endianness.
* **`cais.filesystem`:** Full POSIX.1-2001 directory traversal (`opendir`/`readdir`), metadata inspection (`stat`/`lstat`), and file streaming.
* **`cais.parallel`:** Standard line printer character devices (`/dev/lp0`..`/dev/lp2`) via Centronics block writes.
* **`cais.serial`:** Standard POSIX `termios` serial communications (`/dev/ttyS*`, `/dev/ttyUSB*`, `/dev/ttyACM*`).

---

### 🐧 Subsystem Support by Linux Generation

#### 🔹 Cutting-Edge Linux (Linux 6.7+ .. 6.12+)

* **Kernel:** Linux 6.7+ | **Period:** 2024..2026
* **Network Sockets (`cais.socket`):** ✅ Native Linux `poll(2)` with linear 64-descriptor stack fast-path.
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Full Landlock LSM ABI v4 and v5:
  * Path rules (`LANDLOCK_RULE_PATH_BENEATH`) for filesystem confinement.
  * TCP port binding and connection scoping (`LANDLOCK_ACCESS_NET_*`).
  * Enforced via `prctl(PR_SET_NO_NEW_PRIVS)`.

#### 🔹 Modern LTS Linux (Linux 5.13 .. 6.6)

* **Kernel:** Linux 5.13..6.6 | **Period:** 2021..2023
* **Network Sockets (`cais.socket`):** ✅ Native Linux `poll(2)` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Landlock LSM ABI v1 to v3:
  * Full filesystem path confinement.
  * TCP network scoping uncompiled in kernel (degrades gracefully without failing).

#### 🔹 Classic Linux (Linux 2.2 .. 5.12)

* **Kernel:** 2.2 / 2.4 / 2.6 / 3.x / 4.x / early 5.x | **Period:** 1999..2021
* **Network Sockets (`cais.socket`):** ✅ Native Linux `poll(2)` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** ⚠️ Safe fallback stub (returns `CAIS_SANDBOX_UNSUPPORTED` / `-4`). Landlock was not yet merged into mainline kernel.

#### 🔹 Historic Linux 2.0

* **Kernel:** Linux 2.0.x | **Period:** 1996 (libc5 / early glibc 2.0)
* **Network Sockets (`cais.socket`):** ✅ POSIX `select()` multiplexing (`poll` was introduced in 2.1.23).
* **Kernel Sandboxing (`cais.sandbox`):** ⚠️ Safe fallback stub (returns `CAIS_SANDBOX_UNSUPPORTED` / `-4`).

#### 🔹 Embedded & Specialized Stacks (OpenWrt, Yocto, Android)

* **C Standard Libraries:** musl, uClibc-ng, bionic (Android / Termux)
* **Network Sockets (`cais.socket`):** ✅ Native `poll(2)` across all alternative C standard libraries.
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Dynamic runtime probe: automatically verifies if the underlying host kernel has Landlock compiled in (`CONFIG_SECURITY_LANDLOCK=y`).

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Kernel-Level Sandboxing (`cais.sandbox`)

Linux utilizes the **Landlock LSM** (Linux Security Module), available in mainline Linux kernels starting from version 5.13:

* **Filesystem Rulesets:** Granular path-based confinement using `LANDLOCK_RULE_PATH_BENEATH`. Directories and files are opened via `O_PATH | O_CLOEXEC` to populate rules without granting read/write handles during configuration.
* **TCP Network Rulesets (ABI 4+ / Linux 6.7+):** Enforces `LANDLOCK_ACCESS_NET_BIND_TCP` and `LANDLOCK_ACCESS_NET_CONNECT_TCP` when `network = "none"` is configured.
* **Process Privilege Lockdown:** Calls `prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)` before enforcing the ruleset, preventing child processes or SUID binaries from escalating privileges.
* **Forward ABI Compatibility:** Dynamically queries kernel ABI via `syscall(__NR_landlock_create_ruleset, NULL, 0, LANDLOCK_CREATE_RULESET_VERSION)`. Masks access rights up to the running kernel's ABI level (v1 through v5+), avoiding `EINVAL` when running on older kernels.

### 2. Socket Multiplexing & Networking (`cais.socket`)

* **Standard BSD Sockets:** Full IPv4 TCP stream and UDP datagram support.
* **Linear Array Polling (`poll(2)`):** Bypasses the historic `FD_SETSIZE` (1024) limit inherent to `select()`. Does not suffer from bitmap corruption when socket file descriptors exceed 1023.
* **Zero-Allocation Fast Path:** Utilizes a stack-allocated 64-descriptor scratch buffer (`struct pollfd stack_pfds[64]`) for common polling loops, avoiding dynamic heap allocation during high-frequency I/O loops.
* **Signals:** Network writes use `MSG_NOSIGNAL` in `send()` calls to prevent broken connections from triggering unhandled `SIGPIPE` signals.

### 3. Industrial Serial UART (`cais.serial`)

* **POSIX `termios` Foundation:** Full configuration of baud rates, parity, word size, and stop bits.
* **Mark / Space Parity:** Leverages the Linux kernel extension `CMSPAR` for 9-bit multidrop industrial bus protocols (such as RS-485 / Modbus).
* **High-Speed Baud Rates:** Native support for modern UART baud rates up to 4 Mbaud (`B4000000`).
* **Hardware Modem Signals:** Queries and controls modem status pins (`TIOCMGET`, `TIOCM_CTS`, `TIOCM_DSR`, `TIOCM_CAR`, `TIOCM_RNG`) via `ioctl`.

### 4. Centronics Parallel Port (`cais.parallel`)

* **Character Devices:** Operates directly over standard Linux line printer character devices (`/dev/lp0`, `/dev/lp1`, `/dev/lp2`).
* **Centronics SPP Mode:** Data writes utilize buffered block I/O.
* **Status Monitoring & Drain:** Uses `ioctl(fd, LPGETSTATUS, &status)` to query real-time hardware status pins (`LP_PBUSY`, `LP_PACK`, `LP_POUTPA`, `LP_PSELECD`, `LP_PERRORP`), actively waiting for printer buffer draining.

### 5. Filesystem & Path Operations (`cais.filesystem`)

* Full POSIX.1-2001 directory traversal (`opendir`, `readdir`, `closedir`), metadata interrogation (`lstat`, `stat`), and file streaming.

---

## ⏱️ Version Milestones

| Feature                                      | Minimum Kernel Version | Fallback Behavior                 |
| :------------------------------------------- | :--------------------: | :-------------------------------- |
| **Core I/O & Drivers**                       |       Linux 2.0+       | Fully supported                   |
| **`poll(2)` Multiplexing**                   |     Linux 2.1.23+      | `select()` fallback               |
| **Landlock v1 (Filesystem Path Rules)**      |      Linux 5.13+       | `CAIS_SANDBOX_UNSUPPORTED` (`-4`) |
| **Landlock v2 (File Referring / Renaming)**  |      Linux 5.19+       | Handled mask omitted              |
| **Landlock v3 (File Truncation Control)**    |       Linux 6.2+       | Handled mask omitted              |
| **Landlock v4 (TCP Network Access Scoping)** |       Linux 6.7+       | Network filter omitted            |
| **Landlock v5 (IOCTL Device Control)**       |      Linux 6.12+       | Handled mask omitted              |
