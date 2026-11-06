# Plan 9 from Bell Labs & 9front

`cais` supports **Plan 9 from Bell Labs** (including 3rd Edition, 4th Edition, and modern **9front**), the visionary distributed research operating system where all resources (hardware devices, networks, and processes) are represented as synthetic filesystem directory trees via the **9P protocol**.

---

## 🏛️ Plan 9 Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All Plan 9 Releases)

The following core subsystems are **universally available** across all supported Plan 9 and 9front systems:

* **`cais.platform`:** Architecture identification (x86, ARM, MIPS, SPARC, PowerPC), word size, and Plan 9 system identification.
* **`cais.filesystem`:** Full navigation of synthetic and disk-backed directory trees via standard file descriptors (`open`, `read`, `stat`, `dirread`).
* **`cais.parallel`:** Native synthetic line printer stream `/dev/lp`.
* **`cais.serial`:** Native synthetic serial communication via `/dev/eia0` and `/dev/eia0ctl`.
* **`cais.socket`:** Full TCP/UDP IPv4 networking synthesized over the native `/net/tcp` and `/net/udp` directory hierarchies.
* **`cais.sandbox`:** Safely reports `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with zero runtime overhead (process namespaces in Plan 9 serve as the native boundary).

---

### 🧪 Subsystem Support by Operating System Profile

#### 🔹 9front (Modern Active Distribution)

* **Architectures:** x86_64 (`amd64`), 32-bit x86 (`386`), ARM (`arm`), Raspberry Pi (`arm64`), MIPS
* **Compilers:** Native Plan 9 compilers (`8c`/`6c`/`5c`) or GCC with APE (ANSI/POSIX Environment)
* **Network & Devices (`cais.socket`, `cais.serial`, `cais.parallel`):** ✅ Full native 9P synthetic tree operations.

#### 🔹 Plan 9 3rd & 4th Edition (Bell Labs Releases)

* **Period:** 2000..2002
* **Architectures:** 32-bit x86, MIPS, SPARC, PowerPC
* **Driver Architecture:** Identical pure synthetic device interfaces (`/net` and `/dev`).

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Serial Port Communication via Synthetic Control Streams (`cais.serial`)

Plan 9 eliminates monolithic `ioctl` calls in favor of human-readable text commands written to a dedicated control stream:

* **Data Stream:** Opened at `/dev/eia0` (or `/dev/eia1`) for bidirectional character transmission.
* **Control Stream:** Opened at `/dev/eia0ctl`. `cais` configures UART parameters by writing formatted command strings:
  * Baud rate: `write(ctl_fd, "b115200", 7)`
  * Data bits: `write(ctl_fd, "l8", 2)`
  * Parity: `write(ctl_fd, "pn", 2)` (None), `"pe"` (Even), `"po"` (Odd)
  * Stop bits: `write(ctl_fd, "s1", 2)`
  * Modem lines: `write(ctl_fd, "d1", 2)` (assert DTR), `write(ctl_fd, "r1", 2)` (assert RTS)

### 2. Centronics Parallel Port (`cais.parallel`)

* **Synthetic Device Node:** Directly streams byte payloads to `/dev/lp`. The kernel device driver transparently negotiates the Centronics busy/strobe handshaking.

### 3. Network Sockets via `/net/tcp` and `/net/udp` (`cais.socket`)

Rather than relying on BSD socket syscalls, `cais` maps its socket API to Plan 9's native network directory structures:

* **Connection Lifecycle:**
  1. Open `/net/tcp/clone` to allocate an available network connection channel.
  2. Read the channel identifier from the allocated connection.
  3. Write connection directives (`"connect 192.168.1.100!80"`) to `/net/tcp/<id>/ctl`.
  4. Stream data directly to and from `/net/tcp/<id>/data`.
* **Multiplexing:** Monitored using standard file descriptor polling (`select` under APE or native `rendezvous` mechanisms).

---

## ⏱️ Version Milestones

| Feature                          |     Minimum Environment      | Description & Runtime Behavior                                    |
| :------------------------------- | :--------------------------: | :---------------------------------------------------------------- |
| **Core Architecture & Platform** | **Plan 9 3rd / 4th Edition** | Synthetic namespace execution and CPU architecture detection      |
| **Synthetic Serial & Parallel**  | **Plan 9 3rd / 4th Edition** | Control stream (`/dev/eia*ctl`) and device stream (`/dev/lp`) I/O |
| **Network Sockets**              | **Plan 9 3rd / 4th Edition** | Synthetic `/net/tcp` and `/net/udp` connection management         |
| **Kernel Sandboxing**            |             N/A              | Unsupported (`CAIS_SANDBOX_UNSUPPORTED` / `-4`)                   |
