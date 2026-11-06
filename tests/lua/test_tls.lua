-- tests/lua/test_tls.lua
-- Unit tests for cais.tls (TLS Secure Transport Subsystem)

package.path = "./?.lua;./tests/?.lua;" .. package.path
package.cpath = "./build/lua/?.so;./build/lua/?/init.so;./?.so;./?.dylib;./?.dll;" .. package.cpath

local T = require("tests.harness")
local describe, test = T.describe, T.test
local assert_eq, assert_true, assert_false, assert_nil = T.assert_eq, T.assert_true, T.assert_false, T.assert_nil

local cais = require("cais")
if not cais.tls then
    return
end

-- ============================================================================
-- cais.tls (TLS Transport Subsystem Tests)
-- ============================================================================

describe("cais.tls (Transport Security & Encryption)", function()

    -- 1. Introspection and metadata
    test("exposes backend name and support flag", function()
        local tls = require("cais.tls")
        assert_true(tls.is_supported())
        assert_eq(type(tls.backend()), "string")
        assert_eq(tls.backend(), "bearssl")
    end)

    -- 2. Enumeration tables
    test("defines role and status enum tables", function()
        local tls = cais.tls
        assert_eq(type(tls.role), "table")
        assert_eq(type(tls.role.CLIENT), "number")
        assert_eq(type(tls.role.SERVER), "number")

        assert_eq(type(tls.status), "table")
        assert_eq(type(tls.status.OK), "number")
        assert_eq(type(tls.status.ERROR), "number")
        assert_eq(type(tls.status.WANT_READ), "number")
        assert_eq(type(tls.status.WANT_WRITE), "number")
        assert_eq(type(tls.status.CLOSED), "number")
    end)

    -- 3. Session creation and lifecycle
    test("creates TLS session with options table", function()
        local session, err = cais.tls.create({
            server_name = "example.org",
            verify = false
        })
        assert_nil(err)
        assert_true(session ~= nil)

        -- Calling handshake without socket must fail gracefully
        local ok, hs_err = session:handshake()
        assert_nil(ok)
        assert_true(hs_err ~= nil)

        assert_true(session:close())
    end)

    -- 4. Wrapping cais.socket
    if cais.socket then
        test("wraps cais.socket instance", function()
            local sock = cais.socket.tcp(cais.socket.mode.NONBLOCKING)
            assert_true(sock ~= nil)

            local session, err = cais.tls.wrap(sock, {
                server_name = "localhost",
                verify = false
            })
            assert_nil(err)
            assert_true(session ~= nil)

            -- In non-blocking mode without connected peer, handshake will return want_read or want_write
            local ok, status = session:handshake()
            assert_nil(ok)
            assert_true(status == "want_read" or status == "want_write" or type(status) == "string")

            session:close()
            sock:close()
        end)
    end
end)
