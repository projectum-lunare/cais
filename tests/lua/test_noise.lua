--
-- cais - Communication Abstraction & I/O Substrate
-- Projectum Lunare <https://github.com/projectum-lunare/cais>
-- Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
-- Mozilla Public License Version 2.0
--

package.path = "./?.lua;./tests/?.lua;" .. package.path
package.cpath = "./build/lua/?.so;./build/lua/?/init.so;./?.so;./?.dylib;./?.dll;" .. package.cpath

local T = require("tests.harness")
local describe, test = T.describe, T.test
local assert_eq, assert_true, assert_false, assert_nil = T.assert_eq, T.assert_true, T.assert_false, T.assert_nil

local cais = require("cais")

-- ============================================================================
-- cais.noise (Noise-Based Cryptographic Transport Substrate Tests)
-- ============================================================================
describe("cais.noise (Noise-Based Cryptographic Transport)", function()
    local n = cais.noise

    -- 1. Constants and key generation
    test("defines noise pattern, role and action enum tables", function()
        assert_eq(type(n.pattern), "table")
        assert_eq(n.pattern.XX, 0)
        assert_eq(n.pattern.NN, 1)
        assert_eq(n.pattern.NK, 2)
        assert_eq(n.pattern.IK, 3)

        assert_eq(type(n.role), "table")
        assert_eq(n.role.INITIATOR, 0)
        assert_eq(n.role.RESPONDER, 1)

        assert_eq(type(n.action), "table")
        assert_eq(n.action.WRITE_MSG, 1)
        assert_eq(n.action.READ_MSG, 2)
        assert_eq(n.action.SPLIT, 3)
        assert_eq(n.action.FAILED, -1)
    end)

    test("generates valid 32-byte X25519 keypairs", function()
        local priv, pub = n.keygen()
        assert_eq(type(priv), "string")
        assert_eq(type(pub), "string")
        assert_eq(#priv, 32)
        assert_eq(#pub, 32)
        assert_true(priv ~= pub)
    end)

    -- 2. Noise_NN Handshake
    test("executes complete Noise_NN opportunistic handshake and transport", function()
        local client = n.create({ pattern = "NN", role = "initiator" })
        local server = n.create({ pattern = "NN", role = "responder" })

        assert_eq(client:action(), "write_msg")
        assert_eq(server:action(), "read_msg")
        assert_false(client:is_complete())
        assert_false(server:is_complete())

        -- Step 1: Client writes -> Server reads
        local m1 = client:write_message("client_hello")
        assert_true(type(m1) == "string" and #m1 >= 32)

        local p1 = server:read_message(m1)
        assert_eq(p1, "client_hello")

        -- Step 2: Server writes -> Client reads
        local m2 = server:write_message("server_hello")
        assert_true(type(m2) == "string" and #m2 >= 32)

        local p2 = client:read_message(m2)
        assert_eq(p2, "server_hello")

        -- Handshake complete
        assert_true(client:is_complete())
        assert_true(server:is_complete())
        assert_eq(client:action(), "split")
        assert_eq(server:action(), "split")

        -- Transport: Client -> Server
        local cipher = client:encrypt("Secret NN payload")
        assert_true(type(cipher) == "string" and #cipher == #("Secret NN payload") + 16)

        local plain = server:decrypt(cipher)
        assert_eq(plain, "Secret NN payload")

        -- Transport: Server -> Client
        local reply_cipher = server:encrypt("Server NN reply")
        local reply_plain = client:decrypt(reply_cipher)
        assert_eq(reply_plain, "Server NN reply")

        client:close()
        server:close()
    end)

    -- 3. Noise_XX Handshake (Mutual Authentication & Identity Hiding)
    test("executes Noise_XX handshake with mutual public key verification", function()
        local c_priv, c_pub = n.keygen()
        local s_priv, s_pub = n.keygen()

        local client = n.create({
            pattern = "XX",
            role = "initiator",
            private_key = c_priv
        })
        local server = n.create({
            pattern = "XX",
            role = "responder",
            private_key = s_priv
        })

        -- Msg 1: Client -> Server
        local m1 = client:write_message()
        server:read_message(m1)

        -- Msg 2: Server -> Client
        local m2 = server:write_message("motd")
        local motd = client:read_message(m2)
        assert_eq(motd, "motd")

        -- Msg 3: Client -> Server
        local m3 = client:write_message("auth_done")
        local ack = server:read_message(m3)
        assert_eq(ack, "auth_done")

        -- Verify completion and identity verification
        assert_true(client:is_complete())
        assert_true(server:is_complete())

        assert_eq(client:remote_static(), s_pub)
        assert_eq(server:remote_static(), c_pub)

        -- Bidirectional transport
        local cipher = client:encrypt("Ultra secure message over XX")
        local plain = server:decrypt(cipher)
        assert_eq(plain, "Ultra secure message over XX")

        client:close()
        server:close()
    end)

    -- 4. Noise_NK Handshake (Client Anonymous, Server Static Known)
    test("executes Noise_NK handshake with known server static identity", function()
        local s_priv, s_pub = n.keygen()

        local client = n.create({
            pattern = "NK",
            role = "initiator",
            remote_key = s_pub
        })
        local server = n.create({
            pattern = "NK",
            role = "responder",
            private_key = s_priv
        })

        assert_eq(client:action(), "write_msg")
        assert_eq(server:action(), "read_msg")
        assert_false(client:is_complete())
        assert_false(server:is_complete())

        -- Msg 1 (-> e, es): Client writes -> Server reads
        local m1 = client:write_message("nk_hello")
        local p1 = server:read_message(m1)
        assert_eq(p1, "nk_hello")

        -- Msg 2 (<- e, ee): Server writes -> Client reads
        local m2 = server:write_message("nk_reply")
        local p2 = client:read_message(m2)
        assert_eq(p2, "nk_reply")

        -- Complete
        assert_true(client:is_complete())
        assert_true(server:is_complete())

        -- Client knows server static identity
        assert_eq(client:remote_static(), s_pub)

        -- Bidirectional transport
        local c_data = client:encrypt("Data from anonymous client to known server")
        local p_data = server:decrypt(c_data)
        assert_eq(p_data, "Data from anonymous client to known server")

        local s_data = server:encrypt("Server response to anonymous client")
        local p_back = client:decrypt(s_data)
        assert_eq(p_back, "Server response to anonymous client")

        client:close()
        server:close()
    end)

    -- 5. Noise_IK Handshake (1-RTT WireGuard Pattern with Pre-shared Responder Identity)
    test("executes Noise_IK handshake with pre-configured server key and client auth in 1-RTT", function()
        local c_priv, c_pub = n.keygen()
        local s_priv, s_pub = n.keygen()

        local client = n.create({
            pattern = "IK",
            role = "initiator",
            private_key = c_priv,
            remote_key = s_pub
        })
        local server = n.create({
            pattern = "IK",
            role = "responder",
            private_key = s_priv
        })

        assert_eq(client:action(), "write_msg")
        assert_eq(server:action(), "read_msg")

        -- Msg 1 (-> e, es, s, ss): Client writes -> Server reads
        local m1 = client:write_message("ik_init_payload")
        local p1 = server:read_message(m1)
        assert_eq(p1, "ik_init_payload")

        -- Server learned client identity from Msg 1
        assert_eq(server:remote_static(), c_pub)

        -- Msg 2 (<- e, ee, se): Server writes -> Client reads
        local m2 = server:write_message("ik_reply_payload")
        local p2 = client:read_message(m2)
        assert_eq(p2, "ik_reply_payload")

        -- Complete after 1 RTT
        assert_true(client:is_complete())
        assert_true(server:is_complete())

        assert_eq(client:remote_static(), s_pub)
        assert_eq(server:remote_static(), c_pub)

        -- Bidirectional transport
        local cipher = client:encrypt("WireGuard-style fast session data")
        local plain = server:decrypt(cipher)
        assert_eq(plain, "WireGuard-style fast session data")

        client:close()
        server:close()
    end)

    -- 6. Tamper detection
    test("detects tampering and rejects invalid ciphertext tags", function()
        local client = n.create({ pattern = "NN", role = "initiator" })
        local server = n.create({ pattern = "NN", role = "responder" })

        local m1 = client:write_message()
        server:read_message(m1)
        local m2 = server:write_message()
        client:read_message(m2)

        local cipher = client:encrypt("Authentic Data")
        -- Modify last byte of MAC tag
        local tampered = cipher:sub(1, #cipher - 1) .. string.char(cipher:byte(#cipher) ~ 0x01)

        local plain, err = server:decrypt(tampered)
        assert_nil(plain)
        assert_true(type(err) == "string")

        client:close()
        server:close()
    end)
end)

if arg and arg[0] and arg[0]:match("test_.*%.lua$") then
    os.exit(T.summary())
end
