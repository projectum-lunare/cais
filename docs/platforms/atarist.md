# Atari ST (TOS & FreeMiNT)

`cais` supports the **Atari ST** computer family (Atari 520ST, 1040ST, Mega ST, TT030, Falcon030) powered by Motorola **68000..68060** processors, spanning both the original **TOS (The Operating System)** firmware and the multitasking **FreeMiNT** Unix-like operating system.

---

## 🏛️ Atari ST Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All Atari ST Environments)

The following core subsystems are **universally available** across all Atari ST systems:

* **`cais.platform`:** Motorola 68k architecture detection, 32-bit flat memory addressing, and Big-Endian byte order predicates.
* **`cais.filesystem`:** GEMDOS filesystem operations (`Fsfirst()`, `Fsnext()`, `Fopen()`, `Fread()`, `Fwrite()`, `Fclose()`).
* **`cais.parallel`:** Centronics parallel port printing via the Yamaha YM2149 PSG sound chip port registers and GEMDOS device 0 (`PRN:`).
* **`cais.serial`:** Physical serial communications via direct hardware register mapping of the Motorola **MFP 68901** UART chip.
* **`cais.sandbox`:** Safely reports `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with zero runtime overhead (sandboxing primitives are absent in TOS).

---

### 🕹️ Subsystem Support by Operating Environment

#### 🔹 FreeMiNT (Multitasking Unix-like OS)

* **Kernel:** FreeMiNT 1.12 .. 1.19+ | **Architectures:** 68000..68060, ColdFire (FireBee)
* **Compiler:** `m68k-atari-mint-gcc`
* **Filesystem (`cais.filesystem`):** ✅ Full POSIX-like filesystem with long file names (ext2, minix, VFAT).
* **Network Sockets (`cais.socket`):** ✅ Native **MiNTNet** BSD socket layer (`socket()`, `select()`, IPv4 TCP/UDP).

#### 🔹 Stock TOS (TOS 1.00 .. 4.04 / Falcon030)
* **ROM:** TOS 1.00, 1.02, 1.04 (Rainbow), 2.06, 3.06 (TT), 4.04 (Falcon)
* **Compiler:** `m68k-atari-mint-gcc` (TOS target) or VBCC
* **Filesystem (`cais.filesystem`):** ✅ GEMDOS FAT12/FAT16 file streaming and directory search.
* **Network Sockets (`cais.socket`):** ⚠️ Sockets unavailable (`CAIS_SOCKET_UNSUPPORTED`).

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Serial Port Communication (`cais.serial`)

On Atari ST hardware, serial communication is handled by the **Motorola MC68901 MFP (Multi-Function Peripheral)**:
* **Hardware Register Base:** `0xFFFFFA01` (byte-accessible on odd addresses).
* **Registers:**
  * `UCR` (`0xFFFFFA29`): UART Control Register (data format, parity, clock dividers).
  * `RSR` (`0xFFFFFA2B`): Receiver Status Register (Buffer Full, Overrun, Framing error).
  * `TSR` (`0xFFFFFA2D`): Transmitter Status Register (Buffer Empty, End of Transmission).
  * `UDR` (`0xFFFFFA2F`): UART Data Register (read/write 8-bit character bytes).
* **Baud Rate Clock:** Timer D (`0xFFFFFA25`) is configured directly as the baud rate generator.

### 2. Centronics Parallel Port (`cais.parallel`)

* **Hardware Profile:** The Atari ST parallel port data lines (`D0`–`D7`) are driven directly by Port A of the **Yamaha YM2149 PSG** (Programmable Sound Generator) chip, while the Strobe line is connected to MFP GPIO pin 0.
* **Strobe Generation:** `cais` queries the MFP Busy pin (`MFP + 0x01`), writes the byte to YM2149 Port A (`0xFFFF8800` / `0xFFFF8802`), and toggles the hardware strobe pulse.
* **GEMDOS Fallback:** In user mode or when running under emulators (Hatari, ARAnyM), calls GEMDOS `Bconout(0, ch)`.

### 3. Network Sockets via MiNTNet (`cais.socket`)
* Under FreeMiNT, `cais` interfaces directly with the MiNTNet socket device files (`/dev/socket/`) or native socket syscalls, providing full asynchronous TCP client/server communication and UDP datagram transfers.

---

## ⏱️ Version Milestones

| Feature                            | Minimum Environment  | Description & Runtime Behavior                                         |
| :--------------------------------- | :------------------: | :--------------------------------------------------------------------- |
| **Core Library & Platform**        | **TOS 1.00+** (1985) | Base 68k execution, endianness verification, and GEMDOS initialization |
| **Hardware MFP UART & YM2149 LPT** | **TOS 1.00+** (1985) | Direct register I/O for physical serial and parallel ports             |
| **GEMDOS Filesystem**              | **TOS 1.00+** (1985) | Standard `Fsfirst` / `Fsnext` directory traversal                      |
| **Network Sockets**                |  **FreeMiNT 1.12+**  | Full TCP/UDP IPv4 networking via MiNTNet BSD socket layer              |
| **Kernel Sandboxing**              |         N/A          | Unsupported (`CAIS_SANDBOX_UNSUPPORTED` / `-4`)                        |
