# Noise Cryptographic Transport (`cais.noise`)

The `noise` subsystem provides a lightweight, zero-allocation, certificate-less secure channel engine based on the **Noise Protocol Framework**. It is designed to encrypt, authenticate, and frame bidirectional data streams across modern POSIX systems, Windows, and vintage embedded platforms.

Operating over **Curve25519 (X25519)**, **ChaCha20-Poly1305**, and a native 32-bit implementation of **BLAKE2s**, `cais.noise` establishes cryptographically hardened links over raw TCP sockets (`cais.socket`), physical RS-232 serial UART ports (`cais.serial`), or any generic byte stream.

---

## ⚡ 1. Architectural Principles & Guarantees

### 1.1 Why Noise Instead of TLS?
Standard TLS stacks (such as OpenSSL, mbedTLS, or BearSSL) impose heavy architectural costs:
* **X.509 Certificate Bloat:** Parsing ASN.1, verifying certificate revocation lists (CRLs/OCSP), and maintaining hundreds of trusted Root Certificate Authorities requires hundreds of kilobytes of code and megabytes of memory.
* **Complex State Machines & Handshake Latency:** TLS negotiation involves ciphersuite selection, protocol version negotiation, and multiple round-trips.
* **Heavy Memory Allocations:** Dynamic allocation during certificate validation and record handling is problematic on memory-constrained systems (e.g., DOS, WinCE, embedded microcontrollers).

`cais.noise` solves these issues permanently:
* **Certificate-less Public Key Authentication:** Peers identify each other directly by raw 32-byte Ed25519/X25519 public keys.
* **Fixed Cryptographic Primitives:** Negotiation is eliminated; both peers agree deterministically on the cipher suite.
* **Zero Dynamic Heap Allocations:** Handshake processing and transport encryption operate entirely on fixed stack buffers with zero runtime `malloc()` calls.
* **Compact Footprint:** The entire subsystem compiles into a few dozen kilobytes of static binary code.

---

### 1.2 Native 32-Bit Arithmetic Suite
Rather than using 64-bit BLAKE2b or SHA-512 (which require expensive 64-bit emulation on vintage 32-bit x86, ARMv4/v5, and Motorola 68000 processors), `cais.noise` exclusively standardizes on **BLAKE2s (RFC 7693)**:

$$\text{Noise\_[Pattern]\_25519\_ChaChaPoly\_BLAKE2s}$$

* **Key Exchange (DH):** Curve25519 (X25519) via Monocypher.
* **Authenticated Symmetric Cipher (AEAD):** ChaCha20-Poly1305 with 96-bit Noise-formatted nonces (`uint64` counter padded to 12 bytes).
* **Hash & Key Derivation (KDF):** BLAKE2s operating on 32-bit words, HMAC-BLAKE2s (RFC 2104), and HKDF2 / HKDF3.

---

### 1.3 Transport-Agnostic Framing
Noise produces discrete authenticated cryptographic frames (up to 65,535 bytes). However, TCP sockets and serial UART ports operate as continuous byte streams without packet boundaries.

`cais.noise` includes a transport framing protocol:
```
┌─────────────────────────┬──────────────────────────────────────┐
│  Length Prefix (2 bytes)│  Noise Ciphertext Frame + Tag (MAC)  │
│  Big-Endian (0..65535)  │  Length = Plaintext + 16-byte MAC    │
└─────────────────────────┴──────────────────────────────────────┘
```
This enables transparent, reliable message reassembly over stream sockets and serial links.

---

## 🔄 2. Handshake Patterns

`cais.noise` supports four handshake patterns covering all networking topologies:

### 2.1 `Noise_XX` (Mutual Authentication & Identity Hiding)
The standard recommendation for general-purpose peer-to-peer and client-server communication. Neither party requires prior knowledge of the other's public key; both keys are transmitted encrypted and mutually verified.
```
-> e
<- e, ee, s, es
-> s, se
```
* **Round-Trips:** 1.5 RTT (3 messages).
* **Security:** Mutual static public key authentication, forward secrecy, and identity hiding against passive eavesdroppers.

---

### 2.2 `Noise_NK` (Anonymous Client to Known Server)
Ideal for public web servers and telemetry collectors where the client knows the server's static public key, but the client remains anonymous.
```
Pre-message:
  <- s
Handshake:
  -> e, es
  <- e, ee
```
* **Round-Trips:** 1 RTT (2 messages).
* **Security:** Server authenticated; client identity unauthenticated (anonymous).

---

### 2.3 `Noise_IK` (1-RTT Mutual Auth / WireGuard Pattern)
Optimized for known pairs of peers (like WireGuard VPN tunnels or recurring device communication). The client knows the server's public key, transmits its own encrypted static identity on the very first packet, and completes mutual authentication in a single round-trip.
```
Pre-message:
  <- s
Handshake:
  -> e, es, s, ss
  <- e, ee, se
```
* **Round-Trips:** 1 RTT (2 messages).
* **Security:** Mutual authentication, fast session establishment with zero negotiation delay.

---

### 2.4 `Noise_NN` (Opportunistic Encryption)
Ephemeral-only handshake with no static keys.
```
-> e
<- e, ee
```
* **Round-Trips:** 1 RTT (2 messages).
* **Security:** Defends against passive network sniffing; no defense against active Man-in-the-Middle (MITM) attacks.

---

## ⚙️ 3. Constants & Enumerations

All constants are declared in [`include/cais/noise.h`](file:///home/gustavo/Projetos/cais/include/cais/noise.h).

### 3.1 Handshake Pattern (`cais_noise_pattern_t`)

| C Identifier    | Lua Field (`cais.noise.pattern`) | Value | Description                                     |
| :-------------- | :------------------------------- | :---: | :---------------------------------------------- |
| `CAIS_NOISE_XX` | `XX`                             |  `0`  | Mutual authentication, identity hiding (3 msgs) |
| `CAIS_NOISE_NN` | `NN`                             |  `1`  | Ephemeral only, opportunistic (2 msgs)          |
| `CAIS_NOISE_NK` | `NK`                             |  `2`  | Anonymous initiator, known responder (2 msgs)   |
| `CAIS_NOISE_IK` | `IK`                             |  `3`  | 1-RTT mutual authentication (WireGuard pattern) |

---

### 3.2 Handshake Role (`cais_noise_role_t`)

| C Identifier           | Lua Field (`cais.noise.role`) | Value | Description                                     |
| :--------------------- | :---------------------------- | :---: | :---------------------------------------------- |
| `CAIS_NOISE_INITIATOR` | `INITIATOR`                   |  `0`  | Outbound client initiating the handshake        |
| `CAIS_NOISE_RESPONDER` | `RESPONDER`                   |  `1`  | Inbound server waiting for handshake initiation |

---

### 3.3 State Machine Actions (`cais_noise_action_t`)

| C Identifier                 | Lua String Equivalent | Value | Description                                          |
| :--------------------------- | :-------------------- | :---: | :--------------------------------------------------- |
| `CAIS_NOISE_ACTION_FAILED`   | `"failed"`            | `-1`  | Handshake failed (MAC mismatch or protocol violation)|
| `CAIS_NOISE_ACTION_WRITE_MSG`| `"write_msg"`         |  `1`  | Local session must produce and send next message     |
| `CAIS_NOISE_ACTION_READ_MSG` | `"read_msg"`          |  `2`  | Local session must read incoming handshake message   |
| `CAIS_NOISE_ACTION_SPLIT`    | `"split"`             |  `3`  | Handshake complete; split into transport ciphers     |

---

### 3.4 Buffer Size Constants

| Constant Name               | Value   | Description                                          |
| :-------------------------- | :-----: | :--------------------------------------------------- |
| `CAIS_NOISE_KEY_LEN`        | `32`    | Curve25519 private or public key length in bytes     |
| `CAIS_NOISE_TAG_LEN`        | `16`    | Poly1305 MAC authentication tag length in bytes      |
| `CAIS_NOISE_MAX_FRAME_LEN`  | `65535` | Maximum supported transport payload length per frame |

---

## 🏛️ 4. ANSI C89 Function Reference

### 4.1 Key Generation & CSPRNG

```c
/* Generates a 32-byte X25519 private and public keypair using platform CSPRNG */
cais_status_t cais_noise_keygen(
    unsigned char *out_priv, 
    unsigned char *out_pub, 
    char *err_buf, 
    size_t err_cap
);

/* Seeds the fallback PRNG for platforms lacking native CSPRNG hardware */
void cais_noise_seed(const void *entropy, size_t len);
```

---

### 4.2 Lifecycle & Configuration

```c
/* Allocates a new Noise session state machine */
cais_noise_t *cais_noise_create(
    cais_noise_pattern_t pattern,
    cais_noise_role_t role,
    char *err_buf,
    size_t err_cap
);

/* Configures local static 32-byte private key (computes public key automatically) */
cais_status_t cais_noise_set_static_key(
    cais_noise_t *noise,
    const unsigned char *priv_key,
    char *err_buf,
    size_t err_cap
);

/* Configures pre-known remote static 32-byte public key (for NK or IK) */
cais_status_t cais_noise_set_remote_key(
    cais_noise_t *noise,
    const unsigned char *remote_pub,
    char *err_buf,
    size_t err_cap
);

/* Configures optional contextual prologue bytes mixed into handshake hash */
cais_status_t cais_noise_set_prologue(
    cais_noise_t *noise,
    const void *prologue,
    size_t prologue_len,
    char *err_buf,
    size_t err_cap
);

/* Initializes SymmetricState and enters first handshake state */
cais_status_t cais_noise_start(
    cais_noise_t *noise,
    char *err_buf,
    size_t err_cap
);

/* Securely zeros all sensitive keying material and releases session */
void cais_noise_free(cais_noise_t *noise);
```

---

### 4.3 Introspection

```c
/* Returns current action required by state machine */
cais_noise_action_t cais_noise_get_action(const cais_noise_t *noise);

/* Returns CAIS_TRUE if session has completed handshake and is in transport mode */
cais_bool_t cais_noise_is_complete(const cais_noise_t *noise);

/* Returns pointer to authenticated 32-byte remote public key, or NULL if unavailable */
const unsigned char *cais_noise_get_remote_static(const cais_noise_t *noise);
```

---

### 4.4 Handshake Message Processing (Transport-Agnostic)

```c
/* Writes next handshake message, optionally including cleartext/encrypted payload */
cais_status_t cais_noise_write_message(
    cais_noise_t *noise,
    const void *payload,
    size_t payload_len,
    void *out_msg,
    size_t out_cap,
    size_t *out_len,
    char *err_buf,
    size_t err_cap
);

/* Reads and verifies an incoming handshake message, returning decrypted payload */
cais_status_t cais_noise_read_message(
    cais_noise_t *noise,
    const void *in_msg,
    size_t in_len,
    void *out_payload,
    size_t out_cap,
    size_t *out_payload_len,
    char *err_buf,
    size_t err_cap
);
```

---

### 4.5 Transport Encryption & Decryption

```c
/* Encrypts transport payload with outbound CipherState. Output size: plain_len + 16 */
cais_status_t cais_noise_encrypt(
    cais_noise_t *noise,
    const void *plain,
    size_t plain_len,
    void *out_cipher,
    size_t out_cap,
    size_t *out_len,
    char *err_buf,
    size_t err_cap
);

/* Decrypts and authenticates ciphertext with inbound CipherState. Output size: cipher_len - 16 */
cais_status_t cais_noise_decrypt(
    cais_noise_t *noise,
    const void *cipher,
    size_t cipher_len,
    void *out_plain,
    size_t out_cap,
    size_t *out_len,
    char *err_buf,
    size_t err_cap
);
```

---

### 4.6 Stream Socket Framing Helpers

For direct operation over [`cais.socket`](file:///home/gustavo/Projetos/cais/docs/subsystems/socket.md):

```c
/* Performs full synchronous handshake exchange over connected TCP socket */
cais_status_t cais_noise_handshake_socket(
    cais_noise_t *noise,
    cais_socket_t *sock,
    char *err_buf,
    size_t err_cap
);

/* Encrypts and sends a length-prefixed frame over TCP socket */
cais_status_t cais_noise_send_socket(
    cais_noise_t *noise,
    cais_socket_t *sock,
    const void *data,
    size_t len,
    char *err_buf,
    size_t err_cap
);

/* Receives and decrypts a length-prefixed frame from TCP socket */
cais_status_t cais_noise_recv_socket(
    cais_noise_t *noise,
    cais_socket_t *sock,
    void *out_buf,
    size_t out_cap,
    size_t *out_len,
    char *err_buf,
    size_t err_cap
);
```

---

## 💻 5. Complete C89 Example

```c
#include <stdio.h>
#include <string.h>
#include <cais/cais.h>

int main(void)
{
    char err[128];
    unsigned char server_priv[32], server_pub[32];
    unsigned char client_priv[32], client_pub[32];
    cais_noise_t *client, *server;
    unsigned char msg1[128], msg2[128], msg3[128];
    unsigned char payload[128], cipher[128], decrypted[128];
    size_t mlen = 0, plen = 0, clen = 0, dlen = 0;

    /* Generate Curve25519 static keys for both peers */
    cais_noise_keygen(server_priv, server_pub, err, sizeof(err));
    cais_noise_keygen(client_priv, client_pub, err, sizeof(err));

    /* Initialize Noise_XX sessions */
    client = cais_noise_create(CAIS_NOISE_XX, CAIS_NOISE_INITIATOR, err, sizeof(err));
    server = cais_noise_create(CAIS_NOISE_XX, CAIS_NOISE_RESPONDER, err, sizeof(err));

    cais_noise_set_static_key(client, client_priv, err, sizeof(err));
    cais_noise_set_static_key(server, server_priv, err, sizeof(err));

    cais_noise_start(client, err, sizeof(err));
    cais_noise_start(server, err, sizeof(err));

    /* Handshake Message 1: -> e */
    cais_noise_write_message(client, NULL, 0, msg1, sizeof(msg1), &mlen, err, sizeof(err));
    cais_noise_read_message(server, msg1, mlen, payload, sizeof(payload), &plen, err, sizeof(err));

    /* Handshake Message 2: <- e, ee, s, es */
    cais_noise_write_message(server, NULL, 0, msg2, sizeof(msg2), &mlen, err, sizeof(err));
    cais_noise_read_message(client, msg2, mlen, payload, sizeof(payload), &plen, err, sizeof(err));

    /* Handshake Message 3: -> s, se */
    cais_noise_write_message(client, NULL, 0, msg3, sizeof(msg3), &mlen, err, sizeof(err));
    cais_noise_read_message(server, msg3, mlen, payload, sizeof(payload), &plen, err, sizeof(err));

    /* Both sessions are now in transport phase */
    printf("Handshake complete: %s\n", cais_noise_is_complete(client) ? "YES" : "NO");

    /* Transport Encryption: Client sends to Server */
    cais_noise_encrypt(client, "Hello, C89 Noise!", 17, cipher, sizeof(cipher), &clen, err, sizeof(err));
    cais_noise_decrypt(server, cipher, clen, decrypted, sizeof(decrypted), &dlen, err, sizeof(err));
    decrypted[dlen] = '\0';
    printf("Server received decrypted: '%s'\n", (char *)decrypted);

    cais_noise_free(client);
    cais_noise_free(server);
    return 0;
}
```
