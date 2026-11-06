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
local assert_eq = T.assert_eq

local cais = require("cais")

-- ============================================================================
-- cais lazy umbrella (Deferred Subsystem Initialization)
-- ============================================================================
-- The umbrella defers subsystem initialization to first access (the Lua
-- equivalent of dead-code elimination): require("cais") only registers
-- openers in package.preload, and each subsystem materializes when a field
-- is touched for the first time. Must run before any field is materialized.
describe("cais lazy umbrella (Deferred Subsystem Initialization)", function()
    local fresh

    -- 1. Openers are registered, but nothing is materialized yet
    test("registers openers without materializing subsystems", function()
        assert_eq(type(package.preload["cais.serial"]), "function")
        -- Re-acquire a pristine umbrella: clearing the loaded cache re-runs
        -- the opener, which starts with an empty subsystem surface even when
        -- other suites have already materialized fields on their instance.
        package.loaded["cais"] = nil
        fresh = require("cais")
        assert_eq(rawget(fresh, "platform"), nil)
        assert_eq(rawget(fresh, "serial"), nil)
        assert_eq(rawget(fresh, "socket"), nil)
    end)

    -- 2. First access materializes the field in place
    test("materializes subsystem on first access", function()
        local serial = fresh.serial
        assert_eq(type(serial.open), "function")
        assert_eq(rawget(fresh, "serial") ~= nil, true)
    end)

    -- 3. The fs alias shares the filesystem table identity
    test("fs alias shares filesystem table identity", function()
        assert_eq(fresh.fs == fresh.filesystem, true)
        assert_eq(package.loaded["cais.fs"], package.loaded["cais.filesystem"])
    end)

    -- 4. Unknown fields stay nil without spurious module lookups
    test("unknown fields resolve to nil", function()
        assert_eq(fresh.no_such_subsystem, nil)
        assert_eq(fresh[42], nil)
    end)
end)


-- ============================================================================
-- cais standalone submodules (Independent C Module Loading Tests)
-- ============================================================================
-- Verifies that each cais C submodule (platform, serial, socket, fs, parallel,
-- sandbox) can be independently required as a standalone shared object (.so)
-- without relying on the root 'cais' monolithic module initialization.
describe("cais standalone submodules (Independent C Module Loading)", function()

    -- 1. Independent cais.platform loading
    test("loads cais.platform independently", function()
        local p = require("cais.platform")
        assert_eq(type(p.os), "string")
        assert_eq(p.os, cais.platform.os)
    end)

    -- 2. Independent cais.serial loading
    test("loads cais.serial independently", function()
        local s = require("cais.serial")
        assert_eq(type(s.open), "function")
        assert_eq(s.flow.HARDWARE, 1)
        assert_eq(s.parity.NONE, string.byte("n"))
    end)

    -- 3. Independent cais.socket loading
    test("loads cais.socket independently", function()
        local sock = require("cais.socket")
        assert_eq(type(sock.connect), "function")
    end)

    -- 4. Independent cais.fs loading
    test("loads cais.fs independently", function()
        local fs = require("cais.fs")
        assert_eq(type(fs.exists), "function")
    end)

    -- 5. Independent cais.parallel loading
    test("loads cais.parallel independently", function()
        local par = require("cais.parallel")
        assert_eq(type(par.open), "function")
        assert_eq(par.mode.SPP, 0)
        assert_eq(par.pin.BUSY, 1)
    end)

    -- 6. Independent cais.sandbox loading
    test("loads cais.sandbox independently", function()
        local sbx = require("cais.sandbox")
        assert_eq(type(sbx.is_supported), "function")
    end)

    -- 7. Independent cais.noise loading
    test("loads cais.noise independently", function()
        local ns = require("cais.noise")
        assert_eq(type(ns.keygen), "function")
        assert_eq(ns.pattern.XX, 0)
    end)
end)

if arg and arg[0] and arg[0]:match("test_.*%.lua$") then
    os.exit(T.summary())
end
