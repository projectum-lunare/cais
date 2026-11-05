/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cais/cais.h>

#ifndef CAIS_NO_PLATFORM
int luaopen_cais_platform(lua_State *L);
#endif

#ifndef CAIS_NO_FILESYSTEM
int luaopen_cais_filesystem(lua_State *L);
int luaopen_cais_fs(lua_State *L);
#endif

#ifndef CAIS_NO_PARALLEL
int luaopen_cais_parallel(lua_State *L);
#endif

#ifndef CAIS_NO_SERIAL
int luaopen_cais_serial(lua_State *L);
#endif

#ifndef CAIS_NO_SOCKET
int luaopen_cais_socket(lua_State *L);
#endif

#ifndef CAIS_NO_SANDBOX
int luaopen_cais_sandbox(lua_State *L);
#endif

static int lcais_version(lua_State *L)
{
    lua_pushstring(L, cais_version());
    return 1;
}

static const struct luaL_Reg cais_funcs[] = {
    {"version", lcais_version},
    {NULL, NULL}
};

/*
 * Lazy subsystem registry. The umbrella module registers each subsystem
 * opener in package.preload instead of initializing it eagerly: the Lua
 * equivalent of dead-code elimination, where a consumer only pays for the
 * subsystems it actually touches. The "fs" entry routes through the
 * canonical filesystem module so both names share one table instance.
 */
struct cais_subsystem {
    const char *field;   /* umbrella field name */
    const char *module;  /* canonical require name */
    lua_CFunction openf; /* module opener; NULL routes through require() */
};

static const struct cais_subsystem cais_subsystems[] = {
#ifndef CAIS_NO_PLATFORM
    {"platform", "cais.platform", luaopen_cais_platform},
#endif
#ifndef CAIS_NO_FILESYSTEM
    {"filesystem", "cais.filesystem", luaopen_cais_filesystem},
    {"fs", "cais.fs", NULL},
#endif
#ifndef CAIS_NO_PARALLEL
    {"parallel", "cais.parallel", luaopen_cais_parallel},
#endif
#ifndef CAIS_NO_SERIAL
    {"serial", "cais.serial", luaopen_cais_serial},
#endif
#ifndef CAIS_NO_SOCKET
    {"socket", "cais.socket", luaopen_cais_socket},
#endif
#ifndef CAIS_NO_SANDBOX
    {"sandbox", "cais.sandbox", luaopen_cais_sandbox},
#endif
    {NULL, NULL, NULL}
};

/* require("cais.fs") shim: forwards to the canonical filesystem module. */
static int lcais_preload_fs(lua_State *L)
{
    lua_getglobal(L, "require");
    lua_pushliteral(L, "cais.filesystem");
    lua_call(L, 1, 1);
    return 1;
}

static void preload_submodules(lua_State *L)
{
    const struct cais_subsystem *sub;

    lua_getglobal(L, "package");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "preload");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return;
    }
    for (sub = cais_subsystems; sub->field != NULL; sub++) {
        if (sub->openf != NULL) {
            lua_pushcfunction(L, sub->openf);
        } else {
            lua_pushcfunction(L, lcais_preload_fs);
        }
        lua_setfield(L, -2, sub->module);
    }
    lua_pop(L, 2);
}

/* Metamethod __index: materializes a subsystem on first field access. */
static int lcais_dispatch(lua_State *L)
{
    const struct cais_subsystem *sub;
    const char *key;

    if (lua_type(L, 2) != LUA_TSTRING) {
        lua_pushnil(L);
        return 1;
    }
    key = lua_tostring(L, 2);
    for (sub = cais_subsystems; sub->field != NULL; sub++) {
        if (strcmp(sub->field, key) == 0) {
            lua_getglobal(L, "require");
            if (!lua_isfunction(L, -1)) {
                lua_pop(L, 1);
                lua_pushnil(L);
                return 1;
            }
            lua_pushstring(L, sub->module);
            lua_call(L, 1, 1);
            /* Cache the field so subsequent reads skip the dispatcher. */
            lua_pushvalue(L, -1);
            lua_setfield(L, 1, sub->field);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

int luaopen_cais(lua_State *L)
{
#if LUA_VERSION_NUM >= 502
    luaL_newlib(L, cais_funcs);
#else
    lua_newtable(L);
    luaL_register(L, NULL, cais_funcs);
#endif

    /* Register lazy openers, then install the first-access dispatcher. */
    preload_submodules(L);

    lua_newtable(L);
    lua_pushcfunction(L, lcais_dispatch);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    return 1;
}
