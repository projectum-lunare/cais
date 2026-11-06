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
local assert_eq, assert_true, assert_false = T.assert_eq, T.assert_true, T.assert_false

local cais = require("cais")

-- ============================================================================
-- cais.sandbox (Kernel Security & Process Confinement Tests)
-- ============================================================================
-- Tests the declarative kernel sandboxing bindings across:
-- - Linux: Landlock LSM
-- - Apple Darwin: Seatbelt
-- - Windows NT: Process Mitigations
-- - OpenBSD: pledge & unveil
-- - FreeBSD: Capsicum
--
-- NOTE ON PROCESS ISOLATION:
-- Sandboxing restrictions are permanent for the calling process and cannot be
-- relaxed once locked. Therefore, live enforcement tests execute in a separate
-- child Lua process via os.execute() so that the test runner's own execution
-- environment remains unrestricted.
describe("cais.sandbox (Kernel Security & Isolation)", function()
    local s = cais.sandbox

    -- 1. Introspection of backend engine and version
    test("exposes backend name and version information", function()
        -- s.backend() returns: backend_name_string, abi_version_number, backend_id
        local backend, version, id = s.backend()
        assert_eq(type(backend), "string")
        assert_true(#backend > 0)
        assert_true(version == nil or type(version) == "number")
        assert_true(type(id) == "number")
    end)

    -- 2. API function presence
    test("provides sandbox API function signatures", function()
        assert_eq(type(s.is_supported), "function")
        assert_eq(type(s.apply), "function")
        assert_eq(type(s.backend), "table")
    end)

    -- 3. Path validation
    test("apply with nonexistent path fails visibly without altering sandbox state", function()
        local ok, err, code = s.apply({
            paths = {
                { path = "/nonexistent_sandbox_path_12345", perm = "r" }
            }
        })
        if s.is_supported() and s.backend() == "landlock" then
            assert_false(ok)
            assert_true(err ~= nil)
            assert_eq(code, s.status.ERROR)
        end
    end)

    -- 4. Enum tables and bitmask constants
    test("defines sandbox backend, perm, net and status enum tables", function()
        -- Backend identifiers
        assert_true(s.backend ~= nil)
        assert_eq(s.backend.NONE, 0)
        assert_eq(s.backend.LANDLOCK, 1)
        assert_eq(s.backend.OPENBSD, 2)
        assert_eq(s.backend.CAPSICUM, 3)
        assert_eq(s.backend.SEATBELT, 4)
        assert_eq(s.backend.WIN32, 5)

        -- Permission bitmasks
        assert_true(s.perm ~= nil)
        assert_eq(s.perm.NONE, 0)
        assert_eq(s.perm.READ, 1)
        assert_eq(s.perm.WRITE, 2)
        assert_eq(s.perm.EXEC, 4)

        -- Network access levels
        assert_true(s.net ~= nil)
        assert_eq(s.net.DEFAULT, 0)
        assert_eq(s.net.NONE, 1)
        assert_eq(s.net.LOOPBACK, 2)
        assert_eq(s.net.ALL, 3)

        -- Return status codes
        assert_true(s.status ~= nil)
        assert_eq(s.status.OK, 0)
        assert_eq(s.status.ERROR, -1)
        assert_eq(s.status.UNSUPPORTED, -4)

        -- Convenience top-level permission aliases
        assert_eq(s.READ, 1)
        assert_eq(s.WRITE, 2)
        assert_eq(s.EXEC, 4)
    end)

    -- 5. Live filesystem confinement in child process
    test("enforces filesystem isolation in child process after declarative policy apply", function()
        local backend, ver = s.backend()
        if backend == "landlock" and ver and ver > 0 then
            local lua_bin = os.getenv("LUA_BIN") or "/usr/bin/lua5.5"
            -- Child process applies policy allowing "." (read-only):
            -- 1. Reading "xmake.lua" (inside ".") must succeed.
            -- 2. Reading "/etc/passwd" (outside ".") must be blocked by the kernel.
            local code = [=[
                local s = require("cais.sandbox")
                assert(s.apply({
                    paths = {
                        { path = ".", perm = "r" }
                    }
                }))
                local f1 = io.open("xmake.lua", "r")
                if not f1 then os.exit(1) end
                f1:close()
                local f2 = io.open("/etc/passwd", "r")
                if f2 then f2:close() os.exit(2) end
                os.exit(0)
            ]=]
            local cmd = string.format('LUA_CPATH="./build/lua/?.so;./?.so;;" %s -e %q 2>/dev/null', lua_bin, code)
            local res = os.execute(cmd)
            local status = (res == 0 or res == true)
            assert_true(status)
        end
    end)

    -- 6. Total lockdown without paths
    test("apply without paths in child process restricts entire filesystem", function()
        local backend, ver = s.backend()
        if backend == "landlock" and ver and ver > 0 then
            local lua_bin = os.getenv("LUA_BIN") or "/usr/bin/lua5.5"
            -- Applying empty policy grants zero filesystem paths.
            -- Any attempt to open a file must be denied by Landlock.
            local code = [=[
                local s = require("cais.sandbox")
                assert(s.apply())
                local f = io.open("/etc/passwd", "r")
                if f then f:close() os.exit(2) end
                os.exit(0)
            ]=]
            local cmd = string.format('LUA_CPATH="./build/lua/?.so;./?.so;;" %s -e %q 2>/dev/null', lua_bin, code)
            local res = os.execute(cmd)
            local status = (res == 0 or res == true)
            assert_true(status)
        end
    end)

    -- 7. Object-Oriented Policy Builder & Method Chaining
    test("supports object-oriented policy builder and method chaining", function()
        assert_eq(type(s.policy), "function")
        local p = s.policy()
        assert_eq(type(p), "userdata")

        -- Method chaining returns self
        local ret = p:allow(".", "r")
        assert_eq(ret, p)
        assert_eq(p:allow("/tmp", "rw"), p)
        assert_eq(p:deny_network(), p)
        assert_eq(p:set_network("loopback"), p)
        assert_eq(p:no_child_processes(), p)
        assert_eq(p:set_child_processes(true), p)
        assert_eq(p:no_dynamic_code(), p)
        assert_eq(p:set_dynamic_code(true), p)

        -- tostring inspection
        local str = tostring(p)
        assert_true(str:find("cais.sandbox.policy") ~= nil)

        -- Constructor with options table
        local p2 = s.policy({
            paths = { { path = ".", perm = "r" } },
            network = "none"
        })
        assert_eq(type(p2), "userdata")
    end)

    -- 8. Live enforcement via OOP method chaining in child process
    test("enforces filesystem isolation using OOP policy:apply() in child process", function()
        local backend, ver = s.backend()
        if backend == "landlock" and ver and ver > 0 then
            local lua_bin = os.getenv("LUA_BIN") or "/usr/bin/lua5.5"
            local code = [=[
                local s = require("cais.sandbox")
                local p = s.policy()
                    :allow(".", "r")
                    :deny_network()
                    :no_child_processes()
                assert(p:apply())
                local f1 = io.open("xmake.lua", "r")
                if not f1 then os.exit(1) end
                f1:close()
                local f2 = io.open("/etc/passwd", "r")
                if f2 then f2:close() os.exit(2) end
                os.exit(0)
            ]=]
            local cmd = string.format('LUA_CPATH="./build/lua/?.so;./?.so;;" %s -e %q 2>/dev/null', lua_bin, code)
            local res = os.execute(cmd)
            local status = (res == 0 or res == true)
            assert_true(status)
        end
    end)

    -- 9. Polymorphic cais.sandbox.apply(policy_obj)
    test("cais.sandbox.apply accepts policy userdata polymorphically", function()
        local p = s.policy():allow("/nonexistent_path_xyz", "r")
        local ok, err, code = s.apply(p)
        if s.is_supported() and s.backend() == "landlock" then
            assert_false(ok)
            assert_true(err ~= nil)
            assert_eq(code, s.status.ERROR)
        end
    end)

    -- 10. Unsupported platform handling
    test("unsupported sandbox operations fail visibly on platforms without support", function()
        if not s.is_supported() then
            local ok, err, code = s.apply()
            assert_false(ok)
            assert_eq(type(err), "string")
            assert_eq(code, s.status.UNSUPPORTED)
        end
    end)
end)

if arg and arg[0] and arg[0]:match("test_.*%.lua$") then
    os.exit(T.summary())
end
