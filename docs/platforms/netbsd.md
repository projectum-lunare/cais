# NetBSD & DragonFly BSD

`cais` supports **NetBSD** (renowned for its unmatched hardware portability across dozens of CPU architectures) and **DragonFly BSD** (celebrated for its innovative SMP kernel and HAMMER filesystem).

---

## 🏛️ NetBSD & DragonFly BSD Subsystem Support

### 📦 Universal Baseline Subsystems

The following core subsystems are **universally available** across all releases of NetBSD and DragonFly BSD:

* **`cais.platform`:** Broad architecture support (dozens of CPU targets on NetBSD; x86_64 on DragonFly BSD).
* **`cais.filesystem`:** Full POSIX.1-2001 directory traversal (`opendir`/`readdir`), metadata inspection (`stat`), and file streaming.
* **`cais.parallel`:** Standard line printer character devices (`/dev/lpt0`..`/dev/lpt2`).
* **`cais.serial`:** Standard POSIX `termios` serial communications (`/dev/tty00`, `/dev/tty01`).
* **`cais.socket`:** Native BSD Sockets with `poll(2)` multiplexing.
* **`cais.sandbox`:** Safely reports `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with zero runtime overhead or crashes.

---

### 🚩 Platform Specifics

#### 🔹 NetBSD (All Releases 1.3 .. 10.x+)
* **Architectures:** Unmatched tier of supported architectures (alpha, arm, hppa, m68k, mips, powerpc, riscv, sparc, vax, x86_64).
* **Driver Architecture:** POSIX-compliant file I/O, `termios` serial devices, and linear `poll(2)` array multiplexing.

#### 🔹 DragonFly BSD (All Releases 1.0 .. 6.x+)
* **Architectures:** 64-bit x86 (`x86_64`).
* **Driver Architecture:** High-concurrency BSD socket communication with `poll(2)` and standard POSIX device filesystem trees.

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. High Portability POSIX Foundation

Both NetBSD and DragonFly BSD leverage clean, standards-compliant BSD runtime environments:

* **Filesystem (`cais.filesystem`):** POSIX.1-2001 directory traversal and metadata queries.
* **Networking (`cais.socket`):** Full IPv4 TCP/UDP networking multiplexed through native `poll(2)` with zero-allocation stack scratch buffers.
* **Industrial Serial UART (`cais.serial`):** Standard BSD `termios` supporting USB dongles (`/dev/dtyU*`) and hardware UART ports.
* **Centronics Parallel Port (`cais.parallel`):** Line printer nodes (`/dev/lpt0`, `/dev/lpt1`).

### 2. Kernel Sandboxing Status (`cais.sandbox`)

* **Behavior:** Neither NetBSD nor DragonFly BSD currently exposes an in-kernel declarative process sandbox equivalent to Landlock or Capsicum.
* **Graceful Degradation:** `cais_sandbox_is_supported()` returns `CAIS_FALSE`, and calling `cais_sandbox_apply()` returns `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with an explanatory error message. The application continues running without faults or unhandled signals.

---

## ⏱️ Version Milestones

| Platform          |  Minimum OS Version   | Description                            |
| :---------------- | :-------------------: | :------------------------------------- |
| **NetBSD**        |  NetBSD 1.3+ (1998)   | Full POSIX C89 runtime and BSD sockets |
| **DragonFly BSD** | DragonFly 1.0+ (2004) | Full POSIX C89 runtime and BSD sockets |
