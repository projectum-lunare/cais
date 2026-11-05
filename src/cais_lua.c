/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

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

static void load_submodule(lua_State *L, const char *name, int (*openf)(lua_State *))
{
    lua_pushcfunction(L, openf);
    lua_pushstring(L, name);
    lua_call(L, 1, 1);

    lua_getglobal(L, "package");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "loaded");
        if (lua_istable(L, -1)) {
            lua_pushvalue(L, -3);
            lua_setfield(L, -2, name);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

int luaopen_cais(lua_State *L)
{
#if LUA_VERSION_NUM >= 502
    luaL_newlib(L, cais_funcs);
#else
    lua_newtable(L);
    luaL_register(L, NULL, cais_funcs);
#endif

#ifndef CAIS_NO_PLATFORM
    load_submodule(L, "cais.platform", luaopen_cais_platform);
    lua_setfield(L, -2, "platform");
#endif

#ifndef CAIS_NO_FILESYSTEM
    load_submodule(L, "cais.filesystem", luaopen_cais_filesystem);
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "filesystem");
    lua_setfield(L, -2, "fs");

    lua_getglobal(L, "package");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "loaded");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "cais.filesystem");
            lua_setfield(L, -2, "cais.fs");
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
#endif

#ifndef CAIS_NO_PARALLEL
    load_submodule(L, "cais.parallel", luaopen_cais_parallel);
    lua_setfield(L, -2, "parallel");
#endif

#ifndef CAIS_NO_SERIAL
    load_submodule(L, "cais.serial", luaopen_cais_serial);
    lua_setfield(L, -2, "serial");
#endif

#ifndef CAIS_NO_SOCKET
    load_submodule(L, "cais.socket", luaopen_cais_socket);
    lua_setfield(L, -2, "socket");
#endif

#ifndef CAIS_NO_SANDBOX
    load_submodule(L, "cais.sandbox", luaopen_cais_sandbox);
    lua_setfield(L, -2, "sandbox");
#endif

    return 1;
}
