# IBM OS/2 & ArcaOS

`cais` provides complete, native support for **IBM OS/2** (including OS/2 2.0, OS/2 Warp 3.0, 4.0, and 4.52) and modern **ArcaOS** (the updated OS/2 distribution developed by Arca Noae).

---

## 🏛️ OS/2 & ArcaOS Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All OS/2 Releases)

The following core subsystems are **universally available** across all 32-bit OS/2 and ArcaOS environments:

* **`cais.platform`:** x86 architecture identification, 32-bit flat memory detection, and Little-Endian byte order predicates.
* **`cais.filesystem`:** Full filesystem directory traversal and file streaming via standard OS/2 APIs (`DosFindFirst`, `DosFindNext`) and standard C library routines (`access`, `stat`, `fopen`).
* **`cais.parallel`:** Native line printer device driver access via `DosOpen("LPT1", ...)` and `DosWrite`.
* **`cais.serial`:** Asynchronous communications via OS/2 `DosDevIOCtl` (Category 1).
* **`cais.sandbox`:** Safely reports `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with zero runtime overhead (sandboxing primitives are absent in OS/2).

---

### 🧪 Subsystem Support by OS/2 Generation

#### 🔹 Modern OS/2 (ArcaOS 5.x & OS/2 Warp 4.52)

* **Kernel:** 32-bit OS/2 Kernel (SMP & Uni-Processor) | **Architectures:** 32-bit x86
* **Compilers:** Open Watcom C/C++ (`wcl386 -bt=os2v2`), GCC with EMX runtime
* **Filesystem (`cais.filesystem`):** ✅ Full HPFS and JFS high-performance filesystem traversal.
* **Network Sockets (`cais.socket`):** ✅ Native **IBM TCP/IP Sockets** subsystem (`select()` multiplexing).

#### 🔹 Classic OS/2 (OS/2 Warp 3.0 & 4.0)

* **Period:** 1994..1996
* **Filesystem (`cais.filesystem`):** ✅ Standard FAT16 and HPFS file operations.
* **Network Sockets (`cais.socket`):** ✅ Native IBM TCP/IP Sockets stack (requires TCP/IP component installed).

#### 🔹 Early 32-bit OS/2 (OS/2 2.0 / 2.1)

* **Period:** 1992..1993
* **Filesystem (`cais.filesystem`):** ✅ Base 32-bit flat memory filesystem operations.
* **Parallel & Serial (`cais.parallel`, `cais.serial`):** ✅ Native `DosOpen` and `DosDevIOCtl` drivers.
* **Network Sockets (`cais.socket`):** ⚠️ Sockets unavailable (`CAIS_SOCKET_UNSUPPORTED`).

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Serial Port Communication (`cais.serial`)

On OS/2, serial communications are coordinated through the kernel device driver (`COM1`..`COM4`) via **`DosDevIOCtl`**:

* **Device Handle:** Acquired using `DosOpen("COM1", &hfile, &action, 0, FILE_NORMAL, FILE_OPEN, OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE, 0)`.
* **Category 1 (Async Serial):**
  * `ASYNC_SETBAUDRATE` (`Function 0x41`): Configures baud rate dynamically up to 115200 bps.
  * `ASYNC_SETLINECTRL` (`Function 0x42`): Configures data bits (5–8), stop bits (1–2), and parity.
  * `ASYNC_SETMODEMCTRL` (`Function 0x46`): Controls DTR and RTS modem control lines.
  * `ASYNC_GETMODEMINPUT` (`Function 0x67`): Queries real-time CTS, DSR, and CD hardware signals.

### 2. Centronics Parallel Port (`cais.parallel`)

* **OS/2 Device Driver:** Opens standard `LPT1` through `LPT3` character device names.
* **Buffered Output:** Emits data streams using `DosWrite` directly to the OS/2 spooler or raw hardware driver.

### 3. Network Sockets via IBM TCP/IP (`cais.socket`)

* **IBM BSD Socket Interface:** Links against `tcpip32.lib` (or loads `so32dll.dll` dynamically).
* **Multiplexing:** Uses native `select()` with support for non-blocking TCP streams and asynchronous datagrams.

---

## ⏱️ Version Milestones

| Feature                          |    Minimum Environment    | Description & Runtime Behavior                               |
| :------------------------------- | :-----------------------: | :----------------------------------------------------------- |
| **Core Architecture & Platform** |   **OS/2 2.0+** (1992)    | 32-bit x86 flat memory model execution                       |
| **Filesystem & Device Drivers**  |   **OS/2 2.0+** (1992)    | `DosOpen` LPT devices and `DosDevIOCtl` serial communication |
| **IBM TCP/IP Sockets**           | **OS/2 Warp 3.0+** (1994) | Full TCP/UDP IPv4 networking via native IBM BSD socket stack |
| **Kernel Sandboxing**            |            N/A            | Unsupported (`CAIS_SANDBOX_UNSUPPORTED` / `-4`)              |
