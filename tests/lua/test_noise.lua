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
if not cais.noise then
    return
end

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

    -- 7. Official Cacophony Test Vectors (Snow / Noise Protocol Framework Rev 34)
    local function from_hex(hex)
        return (hex:gsub("..", function(cc)
            return string.char(tonumber(cc, 16))
        end))
    end

    test("matches official Cacophony test vector: Noise_NN_25519_ChaChaPoly_BLAKE2s", function()
        local client = n.create({
            pattern = "NN",
            role = "initiator",
            ephemeral_key = from_hex("893e28b9dc6ca8d611ab664754b8ceb7bac5117349a4439a6b0569da977c464a"),
            prologue = from_hex("4a6f686e2047616c74")
        })
        local server = n.create({
            pattern = "NN",
            role = "responder",
            ephemeral_key = from_hex("bbdb4cdbd309f1a1f2e1456967fe288cadd6f712d65dc7b7793d5e63da6b375b"),
            prologue = from_hex("4a6f686e2047616c74")
        })

        -- Msg 0 (init -> resp)
        local m0 = client:write_message(from_hex("4c756477696720766f6e204d69736573"))
        assert_eq(m0, from_hex("ca35def5ae56cec33dc2036731ab14896bc4c75dbb07a61f879f8e3afa4c79444c756477696720766f6e204d69736573"))
        local p0 = server:read_message(m0)
        assert_eq(p0, from_hex("4c756477696720766f6e204d69736573"))

        -- Msg 1 (resp -> init)
        local m1 = server:write_message(from_hex("4d757272617920526f746862617264"))
        assert_eq(m1, from_hex("95ebc60d2b1fa672c1f46a8aa265ef51bfe38e7ccb39ec5be34069f144808843ff34a6759d06e7733c83aeb5556c15bc762b664b3ba0556b1e7eaea4168bb6"))
        local p1 = client:read_message(m1)
        assert_eq(p1, from_hex("4d757272617920526f746862617264"))

        assert_true(client:is_complete())
        assert_true(server:is_complete())

        local expected_h = from_hex("a621e3943a29c1d984b43727697fbec096107d0b569031ac7e0f1131de19f4f4")
        assert_eq(client:handshake_hash(), expected_h)
        assert_eq(server:handshake_hash(), expected_h)

        -- Msg 2 (init -> resp, transport)
        local c2 = client:encrypt(from_hex("462e20412e20486179656b"))
        assert_eq(c2, from_hex("79285da88da3535f52b07b70006c85706de7ddb1fd3dddac995b7e"))
        local p2 = server:decrypt(c2)
        assert_eq(p2, from_hex("462e20412e20486179656b"))

        -- Msg 3 (resp -> init, transport)
        local c3 = server:encrypt(from_hex("4361726c204d656e676572"))
        assert_eq(c3, from_hex("ffdad3a7f0db4c39077f223659c5c1d107666405566ecdf4ab53bf"))
        local p3 = client:decrypt(c3)
        assert_eq(p3, from_hex("4361726c204d656e676572"))

        -- Msg 4 (init -> resp, transport)
        local c4 = client:encrypt(from_hex("4a65616e2d426170746973746520536179"))
        assert_eq(c4, from_hex("2b9801f5084b9a7e9df57382fb4af099a63cd8ff97bc3284c4c5f28994be58ae46"))
        local p4 = server:decrypt(c4)
        assert_eq(p4, from_hex("4a65616e2d426170746973746520536179"))

        -- Msg 5 (resp -> init, transport)
        local c5 = server:encrypt(from_hex("457567656e2042f6686d20766f6e2042617765726b"))
        assert_eq(c5, from_hex("6c94a97c5de175c870fb9e8d5c50c59d20752b0695baf24e151011ee46a184a65b444e9d97"))
        local p5 = client:decrypt(c5)
        assert_eq(p5, from_hex("457567656e2042f6686d20766f6e2042617765726b"))

        client:close()
        server:close()
    end)

    test("matches official Cacophony test vector: Noise_NK_25519_ChaChaPoly_BLAKE2s", function()
        local client = n.create({
            pattern = "NK",
            role = "initiator",
            ephemeral_key = from_hex("893e28b9dc6ca8d611ab664754b8ceb7bac5117349a4439a6b0569da977c464a"),
            remote_key = from_hex("31e0303fd6418d2f8c0e78b91f22e8caed0fbe48656dcf4767e4834f701b8f62"),
            prologue = from_hex("4a6f686e2047616c74")
        })
        local server = n.create({
            pattern = "NK",
            role = "responder",
            private_key = from_hex("4a3acbfdb163dec651dfa3194dece676d437029c62a408b4c5ea9114246e4893"),
            ephemeral_key = from_hex("bbdb4cdbd309f1a1f2e1456967fe288cadd6f712d65dc7b7793d5e63da6b375b"),
            prologue = from_hex("4a6f686e2047616c74")
        })

        -- Msg 0 (init -> resp)
        local m0 = client:write_message(from_hex("4c756477696720766f6e204d69736573"))
        assert_eq(m0, from_hex("ca35def5ae56cec33dc2036731ab14896bc4c75dbb07a61f879f8e3afa4c794454ae7612d1724af42adb130160a9a94e67b5b169b4e00c189f6467cd17eb7cad"))
        local p0 = server:read_message(m0)
        assert_eq(p0, from_hex("4c756477696720766f6e204d69736573"))

        -- Msg 1 (resp -> init)
        local m1 = server:write_message(from_hex("4d757272617920526f746862617264"))
        assert_eq(m1, from_hex("95ebc60d2b1fa672c1f46a8aa265ef51bfe38e7ccb39ec5be34069f144808843986a5c929337e337ac8b4a074af12ab9f76318a5f18c8b599a443af07383ce"))
        local p1 = client:read_message(m1)
        assert_eq(p1, from_hex("4d757272617920526f746862617264"))

        assert_true(client:is_complete())
        assert_true(server:is_complete())

        local expected_h = from_hex("d7244d974066aae2376f7ba5534f60a6e4e82cd7c9751e226cae3928e6b49f14")
        assert_eq(client:handshake_hash(), expected_h)
        assert_eq(server:handshake_hash(), expected_h)

        -- Msg 2 (init -> resp, transport)
        local c2 = client:encrypt(from_hex("462e20412e20486179656b"))
        assert_eq(c2, from_hex("550027c7a5d450017bcb5e12b8253b1c53fd2213aeda84891d5f95"))
        local p2 = server:decrypt(c2)
        assert_eq(p2, from_hex("462e20412e20486179656b"))

        -- Msg 3 (resp -> init, transport)
        local c3 = server:encrypt(from_hex("4361726c204d656e676572"))
        assert_eq(c3, from_hex("dfbce0c38210ccee35e830aca9dd8b8b3997b933e75bfc8864b759"))
        local p3 = client:decrypt(c3)
        assert_eq(p3, from_hex("4361726c204d656e676572"))

        client:close()
        server:close()
    end)

    test("matches official Cacophony test vector: Noise_IK_25519_ChaChaPoly_BLAKE2s", function()
        local client = n.create({
            pattern = "IK",
            role = "initiator",
            private_key = from_hex("e61ef9919cde45dd5f82166404bd08e38bceb5dfdfded0a34c8df7ed542214d1"),
            ephemeral_key = from_hex("893e28b9dc6ca8d611ab664754b8ceb7bac5117349a4439a6b0569da977c464a"),
            remote_key = from_hex("31e0303fd6418d2f8c0e78b91f22e8caed0fbe48656dcf4767e4834f701b8f62"),
            prologue = from_hex("4a6f686e2047616c74")
        })
        local server = n.create({
            pattern = "IK",
            role = "responder",
            private_key = from_hex("4a3acbfdb163dec651dfa3194dece676d437029c62a408b4c5ea9114246e4893"),
            ephemeral_key = from_hex("bbdb4cdbd309f1a1f2e1456967fe288cadd6f712d65dc7b7793d5e63da6b375b"),
            prologue = from_hex("4a6f686e2047616c74")
        })

        -- Msg 0 (init -> resp)
        local m0 = client:write_message(from_hex("4c756477696720766f6e204d69736573"))
        assert_eq(m0, from_hex("ca35def5ae56cec33dc2036731ab14896bc4c75dbb07a61f879f8e3afa4c79440b03ddc7aac5123d06a1b23b71670e32e76c28239a7ca4ac8f784de7e44c1adbfc6e83fef7352a58d9d56157400c0a737b1d171ce368229c7b752ac25b8faf4eca690f6d896f543be02c996ab2b86b76"))
        local p0 = server:read_message(m0)
        assert_eq(p0, from_hex("4c756477696720766f6e204d69736573"))

        -- Msg 1 (resp -> init)
        local m1 = server:write_message(from_hex("4d757272617920526f746862617264"))
        assert_eq(m1, from_hex("95ebc60d2b1fa672c1f46a8aa265ef51bfe38e7ccb39ec5be34069f144808843d9b5a8927f0ac9655ef76833bc7e5561f42e691ac8404efd6fbd6308b6a27c"))
        local p1 = client:read_message(m1)
        assert_eq(p1, from_hex("4d757272617920526f746862617264"))

        assert_true(client:is_complete())
        assert_true(server:is_complete())

        local expected_h = from_hex("48f3cb8bc9319da4ba1e9933991b1c4ed4034f1f126a76d3a1fbcfd7f94248d4")
        assert_eq(client:handshake_hash(), expected_h)
        assert_eq(server:handshake_hash(), expected_h)

        -- Msg 2 (init -> resp, transport)
        local c2 = client:encrypt(from_hex("462e20412e20486179656b"))
        assert_eq(c2, from_hex("2c256ed08fcd08c2980f954ee4beaccb61c9581340f5dd2fd1cf3b"))
        local p2 = server:decrypt(c2)
        assert_eq(p2, from_hex("462e20412e20486179656b"))

        -- Msg 3 (resp -> init, transport)
        local c3 = server:encrypt(from_hex("4361726c204d656e676572"))
        assert_eq(c3, from_hex("d6033f70eee20945c7c9dba304e397ee3b284ff5e00fd9efb095d3"))
        local p3 = client:decrypt(c3)
        assert_eq(p3, from_hex("4361726c204d656e676572"))

        client:close()
        server:close()
    end)

    test("matches official Cacophony test vector: Noise_XX_25519_ChaChaPoly_BLAKE2s", function()
        local client = n.create({
            pattern = "XX",
            role = "initiator",
            private_key = from_hex("e61ef9919cde45dd5f82166404bd08e38bceb5dfdfded0a34c8df7ed542214d1"),
            ephemeral_key = from_hex("893e28b9dc6ca8d611ab664754b8ceb7bac5117349a4439a6b0569da977c464a"),
            prologue = from_hex("4a6f686e2047616c74")
        })
        local server = n.create({
            pattern = "XX",
            role = "responder",
            private_key = from_hex("4a3acbfdb163dec651dfa3194dece676d437029c62a408b4c5ea9114246e4893"),
            ephemeral_key = from_hex("bbdb4cdbd309f1a1f2e1456967fe288cadd6f712d65dc7b7793d5e63da6b375b"),
            prologue = from_hex("4a6f686e2047616c74")
        })

        -- Msg 0 (init -> resp)
        local m0 = client:write_message(from_hex("4c756477696720766f6e204d69736573"))
        assert_eq(m0, from_hex("ca35def5ae56cec33dc2036731ab14896bc4c75dbb07a61f879f8e3afa4c79444c756477696720766f6e204d69736573"))
        local p0 = server:read_message(m0)
        assert_eq(p0, from_hex("4c756477696720766f6e204d69736573"))

        -- Msg 1 (resp -> init)
        local m1 = server:write_message(from_hex("4d757272617920526f746862617264"))
        assert_eq(m1, from_hex("95ebc60d2b1fa672c1f46a8aa265ef51bfe38e7ccb39ec5be34069f1448088437c365eb362a1c991b0557fe8a7fb187d99346765d93ec63db6c1b01504ebeec55a2298d2dbff80eff034d20595153f63a196a6cead1e11b2bb13e336fa13616dd3e8b0a070c882ed3f1a78c7c06c93"))
        local p1 = client:read_message(m1)
        assert_eq(p1, from_hex("4d757272617920526f746862617264"))

        -- Msg 2 (init -> resp)
        local m2 = client:write_message(from_hex("462e20412e20486179656b"))
        assert_eq(m2, from_hex("46c3307de83b014258717d97781c1f50936d8b7d50c0722a1739654d10392d415b670c114f79b9a4f80541570f77ce88802efa4220cff733e7b5668ba38059ec904b4b8eef9448085faf51"))
        local p2 = server:read_message(m2)
        assert_eq(p2, from_hex("462e20412e20486179656b"))

        assert_true(client:is_complete())
        assert_true(server:is_complete())

        local expected_h = from_hex("6c4c56cf71612f72d05ceb96c0155e6f4ea54a26b504c93de632a2db4a49d200")
        assert_eq(client:handshake_hash(), expected_h)
        assert_eq(server:handshake_hash(), expected_h)

        -- Msg 3 (resp -> init, transport)
        local c3 = server:encrypt(from_hex("4361726c204d656e676572"))
        assert_eq(c3, from_hex("d5e83adfaac5dc324a68f1862df54549e56d209fba707205f328b2"))
        local p3 = client:decrypt(c3)
        assert_eq(p3, from_hex("4361726c204d656e676572"))

        -- Msg 4 (init -> resp, transport)
        local c4 = client:encrypt(from_hex("4a65616e2d426170746973746520536179"))
        assert_eq(c4, from_hex("d102c9029b1f55c788f561ba7737afbccef9c9f1bf2f238167fd40ba9c1c134867"))
        local p4 = server:decrypt(c4)
        assert_eq(p4, from_hex("4a65616e2d426170746973746520536179"))

        -- Msg 5 (resp -> init, transport)
        local c5 = server:encrypt(from_hex("457567656e2042f6686d20766f6e2042617765726b"))
        assert_eq(c5, from_hex("cb1ce80960382c6d5d5e740ffb724d1432f0310b200fb6f8424120f506092744baa415e155"))
        local p5 = client:decrypt(c5)
        assert_eq(p5, from_hex("457567656e2042f6686d20766f6e2042617765726b"))

        client:close()
        server:close()
    end)
end)

if arg and arg[0] and arg[0]:match("test_.*%.lua$") then
    os.exit(T.summary())
end
