---@meta cais.noise

---
---# cais.noise
---
---Noise-Based Cryptographic Transport Substrate (X25519 + ChaCha20-Poly1305 + BLAKE2s).
---Provides zero-allocation, certificate-less, mutual-authentication secure channels.
---
local noise = {}

---Noise Handshake Pattern Constants.
---@enum cais.noise.pattern
noise.pattern = {
    XX = 0, -- Mutual authentication, identity hiding, no pre-shared keys
    NN = 1, -- Ephemeral only, opportunistic encryption (no auth)
    NK = 2, -- Anonymous client, known responder public key
    IK = 3, -- 1-RTT, initiator identity known to responder
}

---Noise Handshake Role Constants.
---@enum cais.noise.role
noise.role = {
    INITIATOR = 0,
    RESPONDER = 1,
}

---Noise Handshake Next Action.
---@enum cais.noise.action
noise.action = {
    WRITE_MSG = 1,
    READ_MSG = 2,
    SPLIT = 3,
    FAILED = -1,
}

---Generates an X25519 keypair using the platform CSPRNG.
---@return string? private_key 32-byte raw binary private key
---@return string? public_key 32-byte raw binary public key
---@return string? error_message
function noise.keygen() end

---Injects external entropy for platforms without native CSPRNG.
---@param entropy string
---@return boolean success
function noise.seed(entropy) end

---@class cais.noise.Options
---@field pattern? cais.noise.pattern|string Handshake pattern: "XX", "NN", "NK", "IK" (default: "XX")
---@field role? cais.noise.role|string Role: "initiator" or "responder" (default: "initiator")
---@field private_key? string 32-byte static private key (required for XX, IK)
---@field remote_key? string 32-byte remote static public key (required for NK, IK initiator)
---@field prologue? string Optional handshake prologue bytes

---Creates a new Noise session state machine.
---@param options? cais.noise.Options
---@return cais.noise.Session? session
---@return string? error_message
function noise.create(options) end

---@class cais.noise.Session
local Session = {}

---Returns the next action required by the session state machine.
---@return "write_msg"|"read_msg"|"split"|"failed" action
function Session:action() end

---Returns true if the handshake is complete and in transport mode.
---@return boolean complete
function Session:is_complete() end

---Returns the 32-byte authenticated remote static public key, if established.
---@return string? remote_public_key
function Session:remote_static() end

---Writes the next handshake message, optionally including payload.
---@param payload? string Optional unencrypted or encrypted payload
---@return string? message_bytes
---@return string? error_message
function Session:write_message(payload) end

---Reads and processes an incoming handshake message, returning any decrypted payload.
---@param message string Incoming message bytes
---@return string? payload_bytes
---@return string? error_message
function Session:read_message(message) end

---Encrypts a transport payload using the outbound CipherState.
---@param plaintext string
---@return string? ciphertext_bytes Plaintext + 16-byte MAC tag
---@return string? error_message
function Session:encrypt(plaintext) end

---Decrypts a transport ciphertext using the inbound CipherState.
---@param ciphertext string Ciphertext with 16-byte MAC tag
---@return string? plaintext_bytes
---@return string? error_message
function Session:decrypt(ciphertext) end

---Closes the session and securely wipes key material from memory.
---@return boolean
function Session:close() end

return noise
