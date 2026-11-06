# KolibriOS

`cais` targets **KolibriOS**, an extraordinarily fast, tiny, open-source 32-bit operating system written entirely in x86 assembly language (forked from MenuetOS).

---

## 🏛️ KolibriOS Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All KolibriOS Releases)

The following core subsystems are **universally available** on KolibriOS 0.7.7.0+:

* **`cais.platform`:** x86 architecture identification, 32-bit word size, and Little-Endian byte order predicates.
* **`cais.parallel`:** Centronics parallel port communication via KolibriOS kernel **Syscall 46** (Hardware I/O port direct access).
* **`cais.serial`:** Physical 16550 UART serial port communication via kernel **Syscall 46**.
* **`cais.socket`:** Full TCP/UDP IPv4 networking via KolibriOS kernel **Syscall 75** (Network Stack).
* **`cais.sandbox`:** Safely reports `CAIS_SANDBOX_UNSUPPORTED` (`-4`) with zero runtime overhead (sandboxing primitives are absent in KolibriOS).

---

### 🧪 Subsystem Support by Operating System Profile

#### 🔹 KolibriOS 0.7.7.0+ (Modern / Current)

* **Architecture:** 32-bit x86
* **Compiler:** `kos32-gcc` (targeting the KolibriOS C library / Newlib)
* **Physical Hardware I/O (`cais.parallel`, `cais.serial`):** ✅ Native Syscall 46 direct port strobe and UART configuration.
* **Network Sockets (`cais.socket`):** ✅ Native Syscall 75 socket operations (TCP loopback, client/server streams, and UDP datagrams).
* **Filesystem (`cais.filesystem`):** ⚠️ Safe fallback stub (file I/O requires custom syscall wrappers).

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Hardware I/O via Syscall 46 (`cais.serial` & `cais.parallel`)

KolibriOS allows user-mode processes to execute direct hardware port reads and writes through **Syscall 46**:

```assembly
mov eax, 46        ; Syscall 46: Port I/O
mov ebx, 0         ; Subfunction 0: Read byte from port (EDX = port)
int 0x40           ; Trigger KolibriOS kernel gate
```

* **Serial Port:** `cais` configures the standard PC UART (`COM1` at `0x3F8`) by writing to the Line Control and Baud Rate Divisor registers directly through Syscall 46.
* **Parallel Port:** Drives the Centronics data and strobe pins on `0x378` / `0x37A` through Syscall 46 block routines.

### 2. Network Sockets via Syscall 75 (`cais.socket`)

KolibriOS provides a native, zero-dependency TCP/IP v4 network stack exposed through **Syscall 75**:

* **Subfunction 0 (Socket Open):** Allocates a kernel socket handle (Stream TCP or Datagram UDP).
* **Subfunction 1 (Connect):** Initiates a TCP connection to a remote IPv4 endpoint.
* **Subfunction 4 (Send) & Subfunction 5 (Receive):** Transfers network buffers directly through kernel memory.
* **Subfunction 8 (Poll):** Asynchronously checks socket readiness.
* **Subfunction 11 (Close):** Gracefully shuts down the connection and frees kernel buffers.

---

## ⏱️ Version Milestones

| Feature | Minimum Environment | Description & Runtime Behavior |
| :--- | :---: | :--- |
| **Core Architecture & Platform** | **KolibriOS 0.7.7.0+** | 32-bit x86 detection and flat-memory execution |
| **Raw Hardware Port Access** | **KolibriOS 0.7.7.0+** | Syscall 46 port I/O for 16550 UART and Centronics printer |
| **TCP/IP Sockets** | **KolibriOS 0.7.7.0+** | Syscall 75 IPv4 network communication |
| **Kernel Sandboxing** | N/A | Unsupported (`CAIS_SANDBOX_UNSUPPORTED` / `-4`) |
