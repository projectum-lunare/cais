-- cais - Physical I/O, System Foundations & Hardware Abstractions
-- Projectum Lunare <https://github.com/projectum-lunare/cais>
-- Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
-- Mozilla Public License Version 2.0

set_project("cais")
set_version("0.3.0")

set_languages("c89")
set_warnings("all", "extra")

add_rules("mode.debug", "mode.release")

-- Public C headers exposed to downstream consumers
add_includedirs("include")

-- Official Projectum Lunare Package Repository & Build Infrastructure Addon
add_repositories("projectum-lunare https://github.com/projectum-lunare/xmake-repo.git")
add_addons("recudo v0.2.8")
includes("@addon/recudo/recudo.lua")

-- ==============================================================================
-- Modular Subsystem & Language Binding Options
-- ==============================================================================
option("lua")
    set_default(true)
    set_showmenu(true)
    set_category("bindings")
    set_description("Build Lua C bindings and runtime modules")
option_end()

option("filesystem")
    set_default(true)
    set_showmenu(true)
    set_category("subsystems")
    set_description("Enable filesystem and path I/O subsystem")
option_end()

option("socket")
    set_default(true)
    set_showmenu(true)
    set_category("subsystems")
    set_description("Enable raw TCP/UDP socket subsystem")
option_end()

option("serial")
    set_default(true)
    set_showmenu(true)
    set_category("subsystems")
    set_description("Enable RS-232 serial UART driver")
option_end()

option("parallel")
    set_default(true)
    set_showmenu(true)
    set_category("subsystems")
    set_description("Enable IEEE 1284 parallel port driver")
option_end()

option("sandbox")
    set_default(true)
    set_showmenu(true)
    set_category("subsystems")
    set_description("Enable kernel security sandboxing")
option_end()

option("noise")
    set_default(true)
    set_showmenu(true)
    set_category("subsystems")
    set_description("Enable Noise cryptographic transport subsystem")
option_end()

-- Propagate exclusion macros for disabled subsystems
if not has_config("filesystem") then add_defines("CAIS_NO_FILESYSTEM") end
if not has_config("socket")     then add_defines("CAIS_NO_SOCKET") end
if not has_config("serial")     then add_defines("CAIS_NO_SERIAL") end
if not has_config("parallel")   then add_defines("CAIS_NO_PARALLEL") end
if not has_config("sandbox")    then add_defines("CAIS_NO_SANDBOX") end
if not has_config("noise")      then add_defines("CAIS_NO_NOISE") end

-- Dependencies for Lua C bindings and C test suites
if has_config("lua") then
    add_requires("lua >=5.4")
end
if has_config("noise") then
    add_requires("monocypher", {configs = {optional = false, shared = false}})
    add_requires("blake2", {configs = {shared = false}})
end
if is_plat("linux", "macosx", "bsd", "windows", "mingw") then
    add_requires("greatest")
end

-- ==============================================================================
-- Platform Driver & Toolchain Declarative Matrix
-- ==============================================================================
local PLATFORMS = {
    -- Vintage & Retro Platforms
    amiga = {
        targetdir  = "build/amiga",
        filesystem = "amiga",
        parallel   = "amiga",
        serial     = "amiga",
        socket     = "bsd",
        sandbox    = "stub",
        language   = "gnu89",
    },
    atari = {
        targetdir  = "build/atarist",
        filesystem = "atarist",
        parallel   = "atarist",
        serial     = "atarist",
        socket     = "bsd",
        sandbox    = "stub",
        language   = "gnu89",
    },
    kolibri = {
        targetdir  = "build/kolibri",
        filesystem = "kolibri",
        parallel   = "kolibri",
        serial     = "kolibri",
        socket     = "kolibri",
        sandbox    = "stub",
        language   = "gnu89",
        cxflags    = {"-nostdlib", "-U_WIN32", "-D__KOLIBRIOS__"},
    },
    dos = {
        targetdir  = "build/dos",
        filesystem = "dos",
        parallel   = "dos",
        serial     = "dos",
        socket     = "bsd",
        sandbox    = "stub",
        rules      = "recudo.watcom",
        watt32     = true,
    },
    os2 = {
        targetdir  = "build/os2",
        filesystem = "os2",
        parallel   = "os2",
        serial     = "os2",
        socket     = "bsd",
        sandbox    = "stub",
        rules      = "recudo.watcom",
    },
    wince = {
        targetdir  = "build/windows_ce",
        filesystem = "windows_ce",
        parallel   = "windows_ce",
        serial     = "windows_ce",
        socket     = "bsd",
        sandbox    = "stub",
        rules      = "recudo.evc3",
        arch_dir   = true,
    },

    -- Windows NT & MinGW
    windows = {
        targetdir  = "build/windows_nt",
        filesystem = "windows_nt",
        parallel   = "windows_nt",
        serial     = "windows_nt",
        socket     = "bsd",
        sandbox    = "windows",
        can_test   = true,
    },
    mingw = {
        targetdir  = "build/windows_nt",
        filesystem = "windows_nt",
        parallel   = "windows_nt",
        serial     = "windows_nt",
        socket     = "bsd",
        sandbox    = "windows",
        can_test   = true,
    },

    -- Plan 9 from Bell Labs
    plan9 = {
        targetdir  = "build/plan9",
        filesystem = "plan9",
        parallel   = "plan9",
        serial     = "plan9",
        socket     = "plan9",
        sandbox    = "stub",
    },

    -- POSIX & Modern Unix (Linux, BSDs, macOS)
    linux = {
        targetdir  = "build/posix",
        filesystem = "posix",
        parallel   = "posix",
        serial     = "posix",
        socket     = "bsd",
        sandbox    = "linux",
        pic        = true,
        can_test   = true,
    },
    openbsd = {
        targetdir  = "build/posix",
        filesystem = "posix",
        parallel   = "posix",
        serial     = "posix",
        socket     = "bsd",
        sandbox    = "openbsd",
        pic        = true,
        can_test   = true,
    },
    freebsd = {
        targetdir  = "build/posix",
        filesystem = "posix",
        parallel   = "posix",
        serial     = "posix",
        socket     = "bsd",
        sandbox    = "freebsd",
        pic        = true,
        can_test   = true,
    },
    bsd = {
        targetdir  = "build/posix",
        filesystem = "posix",
        parallel   = "posix",
        serial     = "posix",
        socket     = "bsd",
        sandbox    = "freebsd",
        pic        = true,
        can_test   = true,
    },
    netbsd = {
        targetdir  = "build/posix",
        filesystem = "posix",
        parallel   = "posix",
        serial     = "posix",
        socket     = "bsd",
        sandbox    = "stub",
        pic        = true,
        can_test   = true,
    },
    dragonflybsd = {
        targetdir  = "build/posix",
        filesystem = "posix",
        parallel   = "posix",
        serial     = "posix",
        socket     = "bsd",
        sandbox    = "stub",
        pic        = true,
        can_test   = true,
    },
    macosx = {
        targetdir  = "build/posix",
        filesystem = "posix",
        parallel   = "stub",
        serial     = "posix",
        socket     = "bsd",
        sandbox    = "darwin",
        pic        = true,
        can_test   = true,
    },
}

local plat_name = get_config("plat") or os.host()
local conf = PLATFORMS[plat_name] or {
    targetdir  = "build/stub",
    filesystem = "stub",
    parallel   = "stub",
    serial     = "stub",
    socket     = "stub",
    sandbox    = "stub",
}
local target_dir = conf.arch_dir and (conf.targetdir .. "/" .. (get_config("arch") or "arm")) or conf.targetdir

local function apply_platform_rules(c)
    if c.language then set_languages(c.language) end
    if c.rules then add_rules(c.rules, {override = true}) end
    if c.cxflags then add_cxflags(c.cxflags, {force = true}) end
    if c.pic then add_cxflags("-fPIC") end
    if c.watt32 then
        local watt32 = os.getenv("WATT32_DIR") or path.join(os.getenv("HOME") or "", ".local/opt/watt32")
        if os.isdir(path.join(watt32, "inc")) then
            add_defines("USE_WATT32")
            add_includedirs(path.join(watt32, "inc"))
        end
    end
end

-- ==============================================================================
-- 1. Static C89 Library (libcais.a / cais.lib) - Multi-Platform Engine
-- ==============================================================================
target("cais_static")
    set_kind("static")
    set_basename("cais")
    set_targetdir(target_dir)
    add_headerfiles("include/(cais/*.h)")
    if has_config("noise") then
        add_packages("monocypher", "blake2")
    end
    add_files(
        "src/cais.c",
        "src/platform/platform.c"
    )
    if has_config("filesystem") then
        add_files(
            "src/filesystem/filesystem.c",
            "src/filesystem/filesystem_" .. conf.filesystem .. ".c"
        )
    end
    if has_config("parallel") then
        add_files("src/parallel/parallel_" .. conf.parallel .. ".c")
    end
    if has_config("serial") then
        add_files("src/serial/serial_" .. conf.serial .. ".c")
    end
    if has_config("socket") then
        add_files(
            "src/socket/socket_common.c",
            "src/socket/socket_" .. conf.socket .. ".c"
        )
    end
    if has_config("sandbox") then
        add_files(
            "src/sandbox/sandbox.c",
            "src/sandbox/sandbox_" .. conf.sandbox .. ".c"
        )
    end
    if has_config("noise") then
        add_files(
            "src/noise/noise_crypto.c",
            "src/noise/noise_protocol.c",
            "src/noise/noise_transport.c"
        )
    end
    apply_platform_rules(conf)

-- ==============================================================================
-- 2. Standalone C Subsystem Libraries (libcais_<subsystem>.a) - Opt-in Modular
-- ==============================================================================
-- Self-contained archives mirroring the Lua standalone submodule design:
-- each library bundles its platform driver plus the shared platform core,
-- so a consumer links exactly one subsystem without the umbrella archive.
-- The dependency graph is a star around platform (no cross-subsystem links),
-- and standard archive link semantics resolve duplicate platform members
-- when several subsystem libraries are combined on the same link line.
local c_subsystems = {
    platform   = {"src/platform/platform.c"},
}
if has_config("filesystem") then
    c_subsystems.filesystem = {"src/filesystem/filesystem.c", "src/filesystem/filesystem_" .. conf.filesystem .. ".c", "src/platform/platform.c"}
end
if has_config("serial") then
    c_subsystems.serial = {"src/serial/serial_" .. conf.serial .. ".c", "src/platform/platform.c"}
end
if has_config("parallel") then
    c_subsystems.parallel = {"src/parallel/parallel_" .. conf.parallel .. ".c", "src/platform/platform.c"}
end
if has_config("socket") then
    c_subsystems.socket = {"src/socket/socket_common.c", "src/socket/socket_" .. conf.socket .. ".c", "src/platform/platform.c"}
end
if has_config("sandbox") then
    c_subsystems.sandbox = {"src/sandbox/sandbox.c", "src/sandbox/sandbox_" .. conf.sandbox .. ".c", "src/platform/platform.c"}
end
if has_config("noise") then
    c_subsystems.noise = {"src/noise/noise_crypto.c", "src/noise/noise_protocol.c", "src/noise/noise_transport.c", "src/platform/platform.c"}
end

for sub_name, files in pairs(c_subsystems) do
    target("cais_" .. sub_name)
        set_kind("static")
        set_group("subsystems")
        set_default(false)
        set_basename("cais_" .. sub_name)
        set_targetdir(target_dir)
        add_headerfiles("include/(cais/*.h)")
        if sub_name == "noise" then
            add_packages("monocypher", "blake2")
        end
        add_files(files)
        apply_platform_rules(conf)
end

-- ==============================================================================
-- 3. Native C API Test Suite
-- ==============================================================================
if is_plat("linux", "macosx", "bsd", "windows", "mingw") then
    target("test_c_api")
        set_kind("binary")
        set_toolchains("recudo-c")
        set_default(false)
        set_targetdir("tests")
        set_rundir(os.projectdir())
        add_files("tests/c/main.c", "tests/c/test_platform.c")
        if has_config("filesystem") then add_files("tests/c/test_filesystem.c") end
        if has_config("sandbox") then add_files("tests/c/test_sandbox.c") end
        if has_config("serial") then add_files("tests/c/test_serial.c") end
        if has_config("parallel") then add_files("tests/c/test_parallel.c") end
        if has_config("socket") then add_files("tests/c/test_socket.c") end
        if has_config("noise") then
            add_files("tests/c/test_noise.c")
            add_packages("monocypher", "blake2")
        end
        add_deps("cais_static")
        add_packages("greatest")
        add_tests("c_api")
        on_install(function (target) end) -- test-only artifact: never packaged
end

-- ==============================================================================
-- 4. Lua Subsystem & C Modules (Powered by recudo.lua.module)
-- ==============================================================================
if has_config("lua") then

    local umbrella_lua_files = {
        "src/cais_lua.c",
        "src/platform/platform_lua.c"
    }
    if has_config("filesystem") then table.insert(umbrella_lua_files, "src/filesystem/filesystem_lua.c") end
    if has_config("serial") then table.insert(umbrella_lua_files, "src/serial/serial_lua.c") end
    if has_config("parallel") then table.insert(umbrella_lua_files, "src/parallel/parallel_lua.c") end
    if has_config("socket") then table.insert(umbrella_lua_files, "src/socket/socket_lua.c") end
    if has_config("sandbox") then table.insert(umbrella_lua_files, "src/sandbox/sandbox_lua.c") end
    if has_config("noise") then table.insert(umbrella_lua_files, "src/noise/noise_lua.c") end

    -- 4.1 Static Lua C Module (libcais_lua.a / cais_lua.lib) for embedding
    target("cais_lua_static")
        add_rules("recudo.lua.module")
        set_values("recudo.lua.kind", "static")
        set_basename("cais_lua")
        add_deps("cais_static")
        add_defines("LUA_USE_C89")
        add_files(umbrella_lua_files)

    if is_plat("linux", "macosx", "bsd", "windows", "mingw") then

    -- 4.2 Umbrella Shared Lua C Module (build/lua/cais.so & ./cais.so)
    target("cais")
        add_rules("recudo.lua.module")
        set_values("recudo.lua.kind", "shared")
        add_deps("cais_static")
        add_defines("LUA_USE_C89")
        add_files(umbrella_lua_files)
        if is_plat("linux", "bsd") then
            -- Hide symbols pulled from the statically embedded Lua runtime: keeps
            -- the module from interposing on a foreign host VM (e.g. LuaJIT) and
            -- trims the dynamic symbol table. macOS uses a two-level namespace and
            -- PE exports are explicit, so both are already safe without this.
            add_shflags("-Wl,--exclude-libs,ALL")
        end
        after_build(function (target)
            if target:kind() ~= "shared" then
                return
            end
            -- Mirror shared module to project root for zero-config require("cais")
            os.trycp(target:targetfile(), path.join(os.projectdir(), path.filename(target:targetfile())))
        end)

    -- 4.3 Standalone C Modules (build/lua/cais/<submodule>.so)
    local submodules = {
        platform   = {"src/platform/platform_lua.c", "src/platform/platform.c"},
    }
    if has_config("filesystem") then
        submodules.filesystem = {"src/filesystem/filesystem_lua.c", "src/filesystem/filesystem.c", "src/filesystem/filesystem_" .. conf.filesystem .. ".c", "src/platform/platform.c"}
        submodules.fs         = {"src/filesystem/filesystem_lua.c", "src/filesystem/filesystem.c", "src/filesystem/filesystem_" .. conf.filesystem .. ".c", "src/platform/platform.c"}
    end
    if has_config("serial") then
        submodules.serial = {"src/serial/serial_lua.c", "src/serial/serial_" .. conf.serial .. ".c", "src/platform/platform.c"}
    end
    if has_config("parallel") then
        submodules.parallel = {"src/parallel/parallel_lua.c", "src/parallel/parallel_" .. conf.parallel .. ".c", "src/platform/platform.c"}
    end
    if has_config("socket") then
        submodules.socket = {"src/socket/socket_lua.c", "src/socket/socket_common.c", "src/socket/socket_" .. conf.socket .. ".c", "src/platform/platform.c"}
    end
    if has_config("sandbox") then
        submodules.sandbox = {"src/sandbox/sandbox_lua.c", "src/sandbox/sandbox.c", "src/sandbox/sandbox_" .. conf.sandbox .. ".c", "src/platform/platform.c"}
    end
    if has_config("noise") then
        submodules.noise = {"src/noise/noise_lua.c", "src/noise/noise_crypto.c", "src/noise/noise_protocol.c", "src/noise/noise_transport.c", "src/platform/platform.c"}
    end

    for mod_name, files in pairs(submodules) do
        target("cais." .. mod_name)
            add_rules("recudo.lua.module")
            set_values("recudo.lua.kind", "shared")
            add_defines("LUA_USE_C89")
            if mod_name == "noise" then
                add_packages("monocypher", "blake2")
            end
            add_files(files)
            if is_plat("linux", "bsd") then
                -- Same interposition guard as the umbrella module (see above).
                add_shflags("-Wl,--exclude-libs,ALL")
            end
    end

    -- 4.4 Lua Test Suite & Portable Launcher (xmake run test_lua)
    local test_lua_deps = {"cais", "cais.platform"}
    if has_config("filesystem") then table.insert(test_lua_deps, "cais.filesystem"); table.insert(test_lua_deps, "cais.fs") end
    if has_config("serial") then table.insert(test_lua_deps, "cais.serial") end
    if has_config("parallel") then table.insert(test_lua_deps, "cais.parallel") end
    if has_config("socket") then table.insert(test_lua_deps, "cais.socket") end
    if has_config("sandbox") then table.insert(test_lua_deps, "cais.sandbox") end
    if has_config("noise") then table.insert(test_lua_deps, "cais.noise") end

    target("test_lua")
        add_rules("recudo.lua.app")
        set_toolchains("recudo-c")
        set_values("recudo.lua.entry", "tests/runner.lua")
        set_default(false)
        add_deps(table.unpack(test_lua_deps))
        on_test(function (target)
            os.execv(path.join(os.projectdir(), "build/bin/test_lua"))
            return true
        end)
        add_tests("lua_suite")
        on_install(function (target) end) -- test-only artifact: never packaged

    end -- is_plat(host)
end -- has_config("lua")

-- ==============================================================================
-- 5. Cross-Platform Smoke Binary
-- ==============================================================================
target("cais_smoke")
    set_kind("binary")
    set_toolchains("recudo-c")
    set_default(false)
    set_rundir(os.projectdir())
    set_targetdir(target_dir)
    add_files("tests/smoke.c")
    add_deps("cais_static")
    apply_platform_rules(conf)
    if conf.can_test then
        set_targetdir("build/bin")
        add_tests("smoke")
    end
    on_install(function (target) end) -- demo binary: never packaged
