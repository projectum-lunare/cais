# Commodore AmigaOS & AROS

`cais` provides first-class support for Commodore's legendary **AmigaOS** operating system family (running on Motorola **68000..68060** processors) and the open-source **AROS Research Operating System**.

---

## 🏛️ AmigaOS Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All AmigaOS Releases)

The following core subsystems are **universally available** across all AmigaOS and AROS environments:

* **`cais.platform`:** Motorola 68k architecture identification, 32-bit pointer detection, and Big-Endian byte order predicates.
* **`cais.filesystem`:** Native AmigaDOS file locking and navigation (`Lock()`, `UnLock()`, `Examine()`, `ExNext()`).
* **`cais.parallel`:** Native Amiga `parallel.device` driver for Centronics printing and data exchange.
* **`cais.serial`:** Native Amiga `serial.device` driver for high-speed serial communications.
* **`cais.sandbox`:** Safely reports `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with zero runtime overhead (sandboxing primitives are absent in AmigaOS).

---

### 🕹️ Subsystem Support by AmigaOS Generation

#### 🔹 Modern AmigaOS & AROS (AmigaOS 3.2 / 3.9 & AROS)

* **Architectures:** Motorola 68020..68060 (AmigaOS) / x86, x86_64, ARM, m68k (AROS)
* **Compilers:** `m68k-amigaos-gcc`, `vbcc`, native AROS GCC
* **Filesystem (`cais.filesystem`):** ✅ Full AmigaDOS directory traversal and buffered file streaming.
* **Network Sockets (`cais.socket`):** ✅ Native **`bsdsocket.library`** (Roadshow, MiamiDX, AmiTCP).

#### 🔹 Classic AmigaOS (AmigaOS 2.04 .. 3.1)

* **Kernel / ROM:** Kickstart 2.04, 3.0, 3.1 | **Architectures:** Motorola 68000..68040
* **Filesystem (`cais.filesystem`):** ✅ Native AmigaDOS 2.0+ standard packet operations.
* **Network Sockets (`cais.socket`):** ✅ Supported via `bsdsocket.library` when an external TCP/IP stack (AmiTCP 3/4 or Miami) is loaded.

#### 🔹 Vintage AmigaOS (AmigaOS 1.2 / 1.3)

* **Kernel / ROM:** Kickstart 1.2 / 1.3 (OASIS / Amiga 500 / 2000)
* **Filesystem (`cais.filesystem`):** ✅ Base AmigaDOS 1.3 filesystem routines.
* **Parallel & Serial (`cais.parallel`, `cais.serial`):** ✅ Native `parallel.device` and `serial.device` standard Exec I/O requests.
* **Network Sockets (`cais.socket`):** ⚠️ Sockets unavailable (`CAIS_SOCKET_UNSUPPORTED`).

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Serial Port Communication (`cais.serial`)

On AmigaOS, `cais` interfaces directly with the standard Exec device subsystem:

* **Exec Device Open:** Opens `serial.device` unit 0 using `CreateMsgPort()` and `CreateIORequest()`.
* **Configuration via `IOExtSer`:** Configures baud rate, parity, stop bits, and data bits via standard Amiga `SDCMD_SETPARAMS` control packets.
* **Hardware Handshaking:** Directly queries and asserts CTS, RTS, and DTR control lines through the Amiga custom chipset (Paula UART and CIA-A / CIA-B interface chips).

### 2. Centronics Parallel Port (`cais.parallel`)

* **Exec Device Interface:** Allocates standard `struct IOStdReq` and opens `parallel.device`.
* **Centronics Handshaking:** Emits raw byte buffers via `CMD_WRITE` with automatic hardware busy/ack handshaking handled by the Amiga CIA (Complex Interface Adapter) 8520 chips.

### 3. Network Sockets via `bsdsocket.library` (`cais.socket`)

* **Dynamic Library Binding:** Opens `bsdsocket.library` at runtime via Exec's `OpenLibrary("bsdsocket.library", 4)`.
* **Zero Host Fragmentation:** Translates `cais.socket` API calls directly into `bsdsocket.library` vector calls (`socket()`, `connect()`, `send()`, `recv()`, `WaitSelect()`), maintaining binary compatibility across Roadshow, Miami, Genesis, and AmiTCP.

---

## ⏱️ Version Milestones

| Feature                                   |  Minimum Environment   | Description & Runtime Behavior                                                    |
| :---------------------------------------- | :--------------------: | :-------------------------------------------------------------------------------- |
| **Core Library & Platform**               | **AmigaOS 1.2** (1986) | Base 68k execution, Kickstart library initialization, and byte-order verification |
| **Device Drivers (`parallel`, `serial`)** | **AmigaOS 1.2** (1986) | Standard Exec asynchronous I/O over `serial.device` and `parallel.device`         |
| **AmigaDOS Filesystem**                   | **AmigaOS 1.2 / 1.3**  | Lock-based file manipulation (`Lock`, `Examine`, `ExNext`)                        |
| **Network Sockets**                       |   **AmigaOS 2.04+**    | Full TCP/UDP IPv4 communication via `bsdsocket.library` (Roadshow / AmiTCP)       |
| **Kernel Sandboxing**                     |          N/A           | Unsupported (`CAIS_SANDBOX_UNSUPPORTED` / `-4`)                                   |
