# Canonical Type System

All subsystems of `cais` are unified by the canonical type system declared in `include/cais/types.h`.

---

## 1. Status Codes (`cais_status_t`)

All fallible non-constructor operations return `cais_status_t`:

```c
typedef enum cais_status {
    CAIS_OK          =  0,  /* Success */
    CAIS_ERROR       = -1,  /* General failure */
    CAIS_AGAIN       = -2,  /* Non-blocking operation would block */
    CAIS_EOF         = -3,  /* End of file / stream reached */
    CAIS_UNSUPPORTED = -4   /* Operation unsupported by host kernel */
} cais_status_t;
```

---

## 2. Canonical Booleans (`cais_bool_t`)

C89 lacks a native `bool` type. Many C libraries resort to returning ambiguous `int` values (where `0` might mean success or false depending on the function).

`cais` enforces an explicit canonical boolean enum:

```c
typedef enum cais_bool {
    CAIS_FALSE = 0,
    CAIS_TRUE  = 1
} cais_bool_t;
```

Functions returning predicates (e.g. `cais_fs_exists`, `cais_platform_is_64bit`, `cais_platform_is_little_endian`) always return `cais_bool_t`.

---

## 3. Physical Signal Line State (`cais_line_state_t`)

Modem and serial UART line queries (such as CTS, DSR, CD, RI) do not return raw integers or booleans, because hardware status lines can be in a high voltage state, low voltage state, or an unreadable error state (e.g., disconnected cable or hardware fault):

```c
typedef enum cais_line_state {
    CAIS_LINE_LOW   = 0,   /* Physical line is LOW / DEASSERTED */
    CAIS_LINE_HIGH  = 1,   /* Physical line is HIGH / ASSERTED */
    CAIS_LINE_ERROR = -1   /* Hardware fault or unsupported pin */
} cais_line_state_t;
```

Example usage:
```c
cais_line_state_t cts = cais_serial_get_cts(port);
if (cts == CAIS_LINE_HIGH) {
    /* Safe to transmit hardware-flowed data */
} else if (cts == CAIS_LINE_LOW) {
    /* Remote device buffer full */
} else {
    /* Driver error or disconnected UART */
}
```
