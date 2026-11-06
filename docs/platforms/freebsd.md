# FreeBSD (Capsicum Capability Mode)

FreeBSD combines high-performance networking, advanced filesystem technologies (ZFS), and the **Capsicum** capability-based security framework. `cais` natively targets FreeBSD on x86_64, aarch64, and riscv64.

---

## 🏛️ FreeBSD Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All FreeBSD Releases)

The following core subsystems are **universally available** across all FreeBSD releases:

* **`cais.platform`:** Architecture identification (x86_64, aarch64, riscv64) and word size.
* **`cais.filesystem`:** Full POSIX.1-2001 directory traversal (`opendir`/`readdir`), metadata inspection (`stat`), and file streaming.
* **`cais.parallel`:** Standard line printer character devices (`/dev/lpt0`..`/dev/lpt2`).
* **`cais.serial`:** POSIX `termios` serial communications (`/dev/cuau*`, `/dev/ttyu*`).
* **`cais.socket`:** High-performance BSD Sockets with `poll(2)` multiplexing.

---

### 😈 Sandboxing Support by FreeBSD Generation

#### 🔹 Modern FreeBSD (FreeBSD 10.0 .. 14.x+)

* **Period:** 2014..2026
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Capsicum capability mode (`cap_enter`):
  * Places the calling process into strict capability mode.
  * Direct filesystem path lookups are denied by the kernel; operations must proceed exclusively through pre-opened file descriptors.
  * Spawning child processes (`fork`) is strictly disallowed.

#### 🔹 Vintage FreeBSD (FreeBSD 3.0 .. 9.x)

* **Period:** 1998..2013
* **Kernel Sandboxing (`cais.sandbox`):** ⚠️ Safe fallback stub (returns `CAIS_SANDBOX_UNSUPPORTED` / `-4`). Capsicum was merged into FreeBSD in version 10.0.

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Kernel-Level Sandboxing (`cais.sandbox`)

FreeBSD implements **Capsicum**, a capability-and-sandbox framework developed by the University of Cambridge and the FreeBSD Foundation:

* **Capability Mode (`cap_enter`):** When `cais_sandbox_apply()` (or `cais.sandbox.apply()`) is invoked on FreeBSD, the process enters Capsicum capability mode via `cap_enter(2)`.
* **Syscall Namespace Lockdown:** Once in capability mode, access to global system namespaces is irrevocably barred: creating new sockets, opening arbitrary filesystem paths by name, or accessing global IPC structures triggers an immediate `ECAPMODE` or `ENOTCAPABLE` error.
* **Pre-opened Descriptor Paradigm:** Descriptors opened *prior* to calling `apply()` (such as active TCP connections or files opened via `cais.fs.open()`) remain accessible, adhering to pure capability discipline.
* **Runtime Capability Probing:** Probes kernel support dynamically via `sysctlbyname("kern.features.capsicum", &val, ...)` with fallback to `cap_getmode()`, ensuring clean operation on custom or embedded FreeBSD kernels compiled without `options CAPABILITIES`.

### 2. Networking & Socket Multiplexing (`cais.socket`)

* **Standard BSD Sockets:** High-performance TCP and UDP IPv4 streams.
* **Linear Array Multiplexing (`poll(2)`):** Bypasses `FD_SETSIZE` constraints with stack-based scratch buffers.

### 3. Industrial Serial UART (`cais.serial`)

* **Call-Out Devices:** Operates over standard FreeBSD call-out nodes (`/dev/cuau0`, `/dev/cuau1` for hardware UARTs, `/dev/cuaU0` for USB adapters).
* **POSIX `termios`:** Full configuration of baud rates, parity, stop bits, and modem signal querying via `ioctl(fd, TIOCMGET, ...)`.

### 4. Parallel Port (`cais.parallel`)

* **Line Printer Driver:** Direct hardware communication over FreeBSD parallel printer nodes (`/dev/lpt0`, `/dev/lpt1`).
* **Hardware Drain:** Polls hardware busy lines until print jobs complete.

---

## ⏱️ Version Milestones

| Feature                    | Minimum FreeBSD Version | Description                                  |
| :------------------------- | :---------------------: | :------------------------------------------- |
| **Core Library & Sockets** |   FreeBSD 3.0+ (1998)   | Standard ANSI C89 and BSD networking         |
| **Capsicum Sandboxing**    |  FreeBSD 10.0+ (2014)   | Native `cap_enter` capability mode isolation |
