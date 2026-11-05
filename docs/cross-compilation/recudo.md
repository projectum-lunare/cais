# Cross-Compilation with Recudo

`cais` integrates [Recudo](https://github.com/projectum-lunare/recudo) to automate cross-compiling the entire matrix of vintage and exotic targets without manual toolchain configuration.

---

## Listing Available Toolchains

```bash
xmake recudo list
```

Recudo inspects local compilers, SDK paths, and emulators (wibo, DOSBox, UAE).

---

## Building Specific Platform Targets

```bash
# Build for modern POSIX (Linux x86_64 / arm64)
xmake recudo posix

# Build for MS-DOS (Open Watcom + WATT-32)
xmake recudo dos

# Build for IBM OS/2 (Open Watcom OS/2)
xmake recudo os2

# Build for Windows CE / Pocket PC (Microsoft eVC 3.0 ARM via wibo)
xmake recudo wince

# Build for Commodore Amiga (m68k-amigaos-gcc)
xmake recudo amiga

# Build for Atari ST (m68k-atari-mint-gcc)
xmake recudo atari

# Build for KolibriOS (kos32-gcc)
xmake recudo kolibri
```

---

## Building the Complete Matrix

To build static libraries for all installed platform toolchains in a single command:

```bash
xmake recudo all
```

Output static libraries will be organized in:
```
build/
├── posix/libcais.a
├── dos/libcais.a
├── os2/libcais.a
├── windows_ce/arm/libcais.a
├── amiga/libcais.a
├── atarist/libcais.a
└── kolibri/libcais.a
```
