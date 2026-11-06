# cais.noise

The `cais.noise` module provides a certificate-less, zero-allocation cryptographic transport substrate powered by the **Noise Protocol Framework** (`Noise_[Pattern]_25519_ChaChaPoly_BLAKE2s`).

Peers establish forward-secure, mutually authenticated encrypted channels with authenticated remote public keys and Poly1305 MAC integrity protection.

---

## Requiring the Module

Available either directly or through the lazy umbrella module:

```lua
-- Standalone direct require:
local noise = require("cais.noise")

-- Or via umbrella cais:
local cais = require("cais")
local noise = cais.noise
```

---

## Enumeration Tables

### `noise.pattern`

Handshake pattern identifiers:

| Pattern Field | Value | Messages | Description |
| :------------ | :---: | :------: | :---------- |
| `XX`          |  `0`  |    3     | **Mutual authentication & identity hiding** (standard recommendation) |
| `NN`          |  `1`  |    2     | **Opportunistic ephemeral encryption** (no static identities) |
| `NK`          |  `2`  |    2     | **Anonymous initiator to known server static key** (API/Web model) |
| `IK`          |  `3`  |    2     | **1-RTT fast mutual authentication** (WireGuard session pattern) |

### `noise.role`

Handshake party role:

* `noise.role.INITIATOR` (`0`): Outbound client initiating the handshake sequence.
* `noise.role.RESPONDER` (`1`): Inbound server awaiting incoming handshake messages.

### `noise.action`

Next expected action by the session state machine:

* `noise.action.WRITE_MSG` (`1`): Session expects `:write_message()` to produce next outbound frame.
* `noise.action.READ_MSG` (`2`): Session expects `:read_message(msg)` with incoming frame bytes.
* `noise.action.SPLIT` (`3`): Handshake completed; session split into independent TX/RX transport ciphers.
* `noise.action.FAILED` (`-1`): Authentication failed or handshake sequence violated.

---

## Module Functions

### `noise.keygen()`

Generates a fresh 32-byte Curve25519 (X25519) keypair using the host platform's hardware CSPRNG (`/dev/urandom` on POSIX or `CryptGenRandom` on Win32).

* **Returns:**
  * `private_key` (32-byte binary string)
  * `public_key` (32-byte binary string)
  * Or `nil, err_msg` on CSPRNG failure.

```lua
local priv, pub = noise.keygen()
assert(#priv == 32 and #pub == 32)
```

---

### `noise.seed(entropy)`

Injects external entropy for platforms without hardware CSPRNG.

* **Parameters:** `entropy` (string).
* **Returns:** `true` on success, or `false, err_msg`.

---

### `noise.create(options)`

Initializes a new Noise session state machine.

* **Parameters:** `options` (table):
  * `pattern` (string or integer, default: `"XX"`): `"XX"`, `"NN"`, `"NK"`, `"IK"`.
  * `role` (string or integer, default: `"initiator"`): `"initiator"` or `"responder"`.
  * `private_key` (string, optional): 32-byte binary static private key (required for `XX`, `IK`, or `NK` responder).
  * `remote_key` (string, optional): 32-byte pre-known remote public key (required for `NK` initiator or `IK` initiator).
  * `prologue` (string, optional): Contextual application bytes to bind cryptographically to the handshake transcript.
* **Returns:** `Session` userdata or `nil, err_msg`.

```lua
local session, err = noise.create({
    pattern = "XX",
    role = "initiator",
    private_key = my_priv_key
})
```

---

## `Session` Userdata Methods

### `session:action()`

Returns the next action string required by the state machine:
`"write_msg"`, `"read_msg"`, `"split"`, or `"failed"`.

---

### `session:is_complete()`

Returns `true` if the handshake has finished successfully and the session is in transport mode.

---

### `session:remote_static()`

Returns the 32-byte binary remote static public key if established and authenticated, or `nil`.

---

### `session:write_message(payload?)`

Writes the next outbound handshake message.

* **Parameters:** `payload` (string, optional): Optional cleartext or encrypted application payload to include in the handshake frame.
* **Returns:** `string` (message bytes) or `nil, err_msg`.

---

### `session:read_message(message)`

Reads and processes an incoming handshake message, authenticating public keys and decrypting payload.

* **Parameters:** `message` (string): Raw incoming message bytes.
* **Returns:** `string` (decrypted payload bytes) or `nil, err_msg`.

---

### `session:encrypt(plaintext)`

Encrypts an application payload using the outbound transport `CipherState` with a 96-bit nonce and Poly1305 MAC tag.

* **Parameters:** `plaintext` (string).
* **Returns:** `string` (ciphertext + 16-byte MAC) or `nil, err_msg`.

---

### `session:decrypt(ciphertext)`

Decrypts and authenticates a transport ciphertext using the inbound `CipherState`.

* **Parameters:** `ciphertext` (string).
* **Returns:** `string` (plaintext bytes) or `nil, err_msg`.

---

### `session:close()`

Securely wipes all cryptographic keys and nonces from memory and closes the session.

---

## 💡 Practical Examples

### 1. `Noise_XX` Mutual Authentication

```lua
local noise = require("cais.noise")

-- Generate identities
local c_priv, c_pub = noise.keygen()
local s_priv, s_pub = noise.keygen()

local client = noise.create({ pattern = "XX", role = "initiator", private_key = c_priv })
local server = noise.create({ pattern = "XX", role = "responder", private_key = s_priv })

-- Msg 1: Client -> Server
local m1 = client:write_message()
server:read_message(m1)

-- Msg 2: Server -> Client
local m2 = server:write_message("welcome_banner")
local banner = client:read_message(m2)

-- Msg 3: Client -> Server
local m3 = client:write_message("client_auth_ready")
server:read_message(m3)

-- Verify mutual identities
assert(client:is_complete() and server:is_complete())
assert(client:remote_static() == s_pub)
assert(server:remote_static() == c_pub)

-- Transport
local cipher = client:encrypt("Sensitive user session payload")
local plain = server:decrypt(cipher)
print("Decrypted payload:", plain)

client:close()
server:close()
```

---

### 2. `Noise_IK` Fast 1-RTT (WireGuard Pattern)

```lua
local noise = require("cais.noise")

local c_priv, c_pub = noise.keygen()
local s_priv, s_pub = noise.keygen()

-- Client pre-configures server public key
local client = noise.create({
    pattern = "IK",
    role = "initiator",
    private_key = c_priv,
    remote_key = s_pub
})
local server = noise.create({
    pattern = "IK",
    role = "responder",
    private_key = s_priv
})

-- Msg 1 (-> e, es, s, ss): Client sends identity encrypted on first packet
local m1 = client:write_message("early_data")
local early = server:read_message(m1)

-- Server can immediately check client's identity
if server:remote_static() ~= c_pub then
    error("Unauthorized client!")
end

-- Msg 2 (<- e, ee, se): Server responds; session is immediately ready
local m2 = server:write_message("session_established")
client:read_message(m2)

assert(client:is_complete() and server:is_complete())
```
