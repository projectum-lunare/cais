# MS-DOS & FreeDOS

`cais` provides battle-tested support for the **MS-DOS** operating system family, ranging from **16-bit Real Mode** (running on vintage Intel 8086 / 80286 systems) to high-performance **32-bit Protected Mode** via DPMI extenders (DOS/4GW, Causeway, CWSDPMI) and modern open-source **FreeDOS**.

---

## 🏛️ MS-DOS Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All DOS Environments)

The following core subsystems are **universally available** across all supported DOS environments:

* **`cais.platform`:** x86 architecture detection, 16-bit vs 32-bit word size detection, and little-endian byte ordering.
* **`cais.filesystem`:** Native DOS FAT filesystem I/O via standard DOS API handles (`_dos_open`, `_dos_read`, `_dos_write`, `_dos_close`).
* **`cais.parallel`:** Native IEEE 1284 / Centronics printing via BIOS `INT 17h` and direct I/O port toggling (`0x378` / `0x278`).
* **`cais.serial`:** Physical serial communications via direct 8250 / 16550 UART port manipulation and BIOS `INT 14h`.
* **`cais.sandbox`:** Safely reports `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with zero runtime overhead (sandboxing primitives are absent in DOS).

---

### 🕹️ Subsystem Support by DOS Environment

#### 🔹 32-bit Protected Mode (DJGPP & Open Watcom DOS/4GW)

* **Environment:** 386+ protected mode with DPMI host
* **Compilers:** DJGPP (`i586-pc-msdosdjgpp-gcc`), Open Watcom C/C++ (`wcl386`)
* **Filesystem (`cais.filesystem`):** ✅ Full flat-memory file streaming and directory traversal.
* **Network Sockets (`cais.socket`):** ✅ Native **WATT-32** TCP/IP stack (`select_s()` multiplexing, packet drivers, Ethernet/SLIP/PPP).

#### 🔹 16-bit Real Mode (MS-DOS 3.30 .. 6.22)

* **Environment:** 8086, 80286, 80386 real mode (segmented memory `CS:IP`)
* **Compilers:** Open Watcom C (`wcl -ms` / `-ml`), Borland Turbo C++
* **Filesystem (`cais.filesystem`):** ✅ Near/far memory buffer streaming and DOS interrupt file access.
* **Network Sockets (`cais.socket`):** ⚠️ Sockets unavailable (`CAIS_SOCKET_UNSUPPORTED`).

#### 🔹 FreeDOS (1.2 / 1.3)

* **Environment:** 16-bit or 32-bit protected mode running under the FreeDOS kernel
* **Driver Architecture:** Identical to standard MS-DOS; supports both real-mode BIOS services and protected-mode WATT-32 networking.

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Serial Port Communication (`cais.serial`)

On DOS, `cais` bypasses high-overhead DOS interrupts when configured for hardware mode, writing directly to the National Semiconductor **8250 / 16550 UART** I/O registers:

* **Base Addresses:** `COM1` (`0x3F8`), `COM2` (`0x2F8`), `COM3` (`0x3E8`), `COM4` (`0x2E8`).
* **Line Control Register (LCR):** Programmed directly for data bits (5–8), stop bits (1–2), and parity (None, Even, Odd).
* **Baud Rate Divisor:** Divisor Latch Access Bit (`DLAB`) toggling to load 16-bit baud rate divisors into `DLL` and `DLM` (115200 Hz base clock).
* **Modem Control:** Hardware DTR and RTS pin assertion via `MCR` register (`0x3FC`).

### 2. Centronics Parallel Port (`cais.parallel`)

* **Hardware Port I/O:** Directly controls standard LPT hardware addresses (`0x378`, `0x278`, `0x3BC`).
* **SPP Protocol Strobe:** Writes 8-bit ASCII characters to the Data Register (`PORT + 0`), queries Busy and Acknowledge flags on the Status Register (`PORT + 1`), and pulses the hardware Strobe pin on the Control Register (`PORT + 2`).
* **BIOS Fallback:** Safely falls back to BIOS `INT 17h` (`AH=00h` print character, `AH=02h` get status) when direct hardware port access is restricted.

### 3. Network Sockets via WATT-32 (`cais.socket`)

* **Ethernet Packet Driver:** Operates over standard software interrupt `0x60` packet drivers.
* **BSD Sockets Emulation:** In 32-bit mode, maps standard socket descriptors to WATT-32 internal structures, supporting non-blocking TCP streams, UDP datagrams, and multiplexed polling via `select_s()`.

---

## ⏱️ Version Milestones

| Feature                        |    Minimum Environment    | Description & Runtime Behavior                                      |
| :----------------------------- | :-----------------------: | :------------------------------------------------------------------ |
| **Core Library & Platform**    |  **MS-DOS 3.30+** (1987)  | Base 16-bit real mode execution and CPU detection                   |
| **Direct Hardware UART & LPT** |  **MS-DOS 3.30+** (1987)  | Direct port I/O (`inportb`/`outportb`) across standard PC addresses |
| **32-bit Protected Mode**      |  **MS-DOS 5.0+** / 386+   | Flat memory model execution via DOS/4GW, CWSDPMI, or Causeway       |
| **WATT-32 Network Sockets**    | **32-bit Protected Mode** | Full TCP/UDP IPv4 networking over hardware packet drivers           |
| **Kernel Sandboxing**          |            N/A            | Unsupported (`CAIS_SANDBOX_UNSUPPORTED` / `-4`)                     |
