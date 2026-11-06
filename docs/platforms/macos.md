# macOS (Darwin & Apple Silicon)

`cais` supports Apple's Darwin operating system family across **PowerPC (G3/G4/G5)**, **Intel (x86/x86_64)**, and **Apple Silicon (arm64)** architectures, ranging from vintage Mac OS X releases to modern macOS.

---

## 🏛️ macOS Version & Subsystem Support

### 📦 Universal Baseline Subsystems (All Darwin Releases)

The following core subsystems are **universally available** across all supported versions of Apple Darwin:

* **`cais.platform`:** Architecture detection (PowerPC, Intel x86/x86_64, Apple Silicon arm64), bitness, and endianness predicates.
* **`cais.filesystem`:** Full POSIX.1-2001 directory traversal (`opendir`/`readdir`), metadata inspection (`stat`), and file streaming.
* **`cais.serial`:** Standard POSIX `termios` serial communications (`/dev/cu.*` and `/dev/tty.*`).
* **`cais.parallel`:** Safely reports `CAIS_PARALLEL_UNSUPPORTED` (`-4`) with zero runtime overhead (hardware not present on Mac).

---

### 🍏 Subsystem Support by macOS Generation

#### 🔹 Modern macOS (macOS 11 Big Sur .. macOS 15 Sequoia)
* **Kernel:** Darwin 20..24 | **Period:** 2020..2026
* **Architectures:** Apple Silicon (`arm64`), Intel (`x86_64`)
* **Network Sockets (`cais.socket`):** ✅ Native Darwin `poll(2)` with linear 64-descriptor stack fast-path.
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Apple Seatbelt framework via dynamic TinyScheme SBPL synthesis and `sandbox_init()`.

#### 🔹 64-bit Intel Era (OS X 10.8 Mountain Lion .. macOS 10.15 Catalina)
* **Kernel:** Darwin 12..19 | **Period:** 2012..2019
* **Architectures:** Intel 64-bit (`x86_64`) exclusively
* **Network Sockets (`cais.socket`):** ✅ Native Darwin `poll(2)` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Full Seatbelt SBPL sandboxing (`sandbox_init()`).

#### 🔹 Intel 32/64-bit Transition (Mac OS X 10.6 Snow Leopard .. 10.7 Lion)
* **Kernel:** Darwin 10..11 | **Period:** 2009..2011
* **Architectures:** Intel 32-bit (`i386`) and 64-bit (`x86_64`) (dropped PowerPC support)
* **Network Sockets (`cais.socket`):** ✅ Native Darwin `poll(2)` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Full Seatbelt SBPL sandboxing (`sandbox_init()`).

#### 🔹 The PowerPC & Seatbelt Debut (Mac OS X 10.5 Leopard)
* **Kernel:** Darwin 9 | **Period:** 2007
* **Architectures:** PowerPC (`ppc`/`ppc64`, G4/G5) and Intel (`i386`/`x86_64`) Universal Binary
* **Network Sockets (`cais.socket`):** ✅ Native Darwin `poll(2)` multiplexing.
* **Kernel Sandboxing (`cais.sandbox`):** ✅ Seatbelt framework debut — supported natively on both PowerPC and Intel!
* *Note:* Leopard was Apple's last operating system release to support PowerPC hardware.

#### 🔹 Early Vintage Mac OS X (Mac OS X 10.0 Cheetah .. 10.4 Tiger)
* **Kernel:** Darwin 1..8 | **Period:** 2001..2005
* **Architectures:** PowerPC (`ppc`, G3/G4/G5) and early Intel (10.4)
* **Network Sockets (`cais.socket`):**
  * Mac OS X 10.3..10.4 (Panther, Tiger): ✅ Native `poll(2)`.
  * Mac OS X 10.0..10.2 (Cheetah, Puma, Jaguar): ✅ BSD `select()`.
* **Kernel Sandboxing (`cais.sandbox`):** ⚠️ Safe fallback stub (returns `CAIS_SANDBOX_UNSUPPORTED` / `-4`). Seatbelt was not yet introduced into the XNU kernel.

---

## 🏛️ Driver & Subsystem Implementation Details

### 1. Kernel-Level Sandboxing (`cais.sandbox`)

macOS uses the **Seatbelt** kernel MAC (Mandatory Access Control) framework, known commercially as the foundation of **App Sandbox**:

* **Dynamic SBPL Generation:** Rather than relying on static Xcode Entitlements (which cannot be configured in dynamic CLI utilities or runtime environments), `cais` dynamically synthesizes a **Sandbox Profile Language (SBPL)** script in TinyScheme syntax:
  ```scheme
  (version 1)
  (deny default)
  (allow file-read-metadata)
  (allow sysctl-read)
  (allow mach-lookup)
  (allow signal (target self))
  (deny network*)                                   ;; if network == "none"
  (deny process-fork)                              ;; if child_processes == false
  (allow file-read* (subpath "/path/to/read"))      ;; read allowed paths
  (allow file-read* file-write* (subpath "/tmp"))   ;; write allowed paths
  ```
* **Enforcement via `sandbox_init`:** Passes the generated profile directly to the kernel via `sandbox_init(profile, 0, &err_msg)`.
* **Zero Host Dependencies:** Invokes `sandbox_init` from `libSystem` without external library linkages.

### 2. Network Sockets (`cais.socket`)

* **Standard BSD Sockets:** Full TCP and UDP IPv4 communication.
* **`poll(2)` Multiplexing:** Uses native Darwin `poll(2)` with a 64-descriptor stack fast-path buffer, bypassing `FD_SETSIZE` limits.
* **Darwin `SIGPIPE` Handling:** Because the Linux `MSG_NOSIGNAL` send flag is not implemented in the XNU kernel, `cais` protects socket streams against unexpected process termination.

### 3. Serial Port Communication (`cais.serial`)

* **POSIX `termios` Implementation:** Supports standard USB serial adapters and FTDI dongles on Darwin.
* **Device Conventions:** Standard macOS call-out (`/dev/cu.*`) and dial-in (`/dev/tty.*`) device nodes.
* **Modem Line Control:** Direct `ioctl` query and assertion of `TIOCM_DTR`, `TIOCM_RTS`, `TIOCM_CTS`, and `TIOCM_CD`.

### 4. Parallel Port (`cais.parallel`)

* **Hardware Profile:** Physical IEEE 1284 / Centronics parallel ports were never part of standard Apple Macintosh hardware.
* **Behavior:** Returns `CAIS_PARALLEL_UNSUPPORTED` (`-4`) with zero runtime overhead or crashes.

---

## ⏱️ Version Milestones

| Feature                    |    Minimum OS Version    | Description                                                                                       |
| :------------------------- | :----------------------: | :------------------------------------------------------------------------------------------------ |
| **Core Library & Sockets** |      Mac OS X 10.0+      | Base POSIX and BSD socket operations                                                              |
| **PowerPC Architecture**   |   Mac OS X 10.0..10.5    | Suporte nativo a PowerPC (G3/G4/G5); Mac OS X 10.5 Leopard foi a última versão Apple para PowerPC |
| **`poll(2)` Polling**      | Mac OS X 10.3+ (Panther) | Native linear descriptor array polling                                                            |
| **Seatbelt Sandboxing**    | Mac OS X 10.5+ (Leopard) | Dynamic `sandbox_init` and SBPL policy compiler                                                   |
| **Apple Silicon (arm64)**  |  macOS 11.0+ (Big Sur)   | Native 64-bit ARM execution                                                                       |
