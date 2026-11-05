/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
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

#include <cais/platform.h>

#define SET_BOOL(key, val)                                                                         \
    do {                                                                                           \
        lua_pushboolean(L, (val) ? 1 : 0);                                                         \
        lua_setfield(L, -2, (key));                                                                \
    } while (0)


int luaopen_cais_platform(lua_State *L);

int luaopen_cais_platform(lua_State *L)
{
    lua_newtable(L);

    lua_pushstring(L, CAIS_OS_NAME);
    lua_setfield(L, -2, "os");
    lua_pushstring(L, CAIS_ARCH_NAME);
    lua_setfield(L, -2, "arch");
    lua_pushinteger(L, (lua_Integer)CAIS_BITS);
    lua_setfield(L, -2, "bits");
    lua_pushstring(L, CAIS_GET_ENDIAN_NAME());
    lua_setfield(L, -2, "endian");

    SET_BOOL("is_64bit", CAIS_IS_64BIT);
    SET_BOOL("is_32bit", CAIS_IS_32BIT);
    SET_BOOL("is_16bit", CAIS_IS_16BIT);

    SET_BOOL("is_little_endian", CAIS_IS_LITTLE_ENDIAN);
    SET_BOOL("is_big_endian", CAIS_IS_BIG_ENDIAN);

    SET_BOOL("is_posix", CAIS_IS_POSIX);
    SET_BOOL("is_windows", CAIS_IS_WINDOWS);
    SET_BOOL("is_windows_nt", CAIS_IS_WINDOWS_NT);
    SET_BOOL("is_windows_ce", CAIS_IS_WINDOWS_CE);
    SET_BOOL("is_amiga", CAIS_IS_AMIGA);
    SET_BOOL("is_amiga_classic", CAIS_IS_AMIGA_CLASSIC);
    SET_BOOL("is_aros", CAIS_IS_AROS);
    SET_BOOL("is_amigaos4", CAIS_IS_AMIGAOS4);
    SET_BOOL("is_linux", CAIS_IS_LINUX);
    SET_BOOL("is_darwin", CAIS_IS_DARWIN);
    SET_BOOL("is_bsd", CAIS_IS_BSD);
    SET_BOOL("is_haiku", CAIS_IS_HAIKU);
    SET_BOOL("is_beos", CAIS_IS_BEOS);
    SET_BOOL("is_plan9", CAIS_IS_PLAN9);
    SET_BOOL("is_kolibri", CAIS_IS_KOLIBRI);
    SET_BOOL("is_minix", CAIS_IS_MINIX);
    SET_BOOL("is_dos", CAIS_IS_DOS);
    SET_BOOL("is_os2", CAIS_IS_OS2);
    SET_BOOL("is_atarist", CAIS_IS_ATARIST);
    SET_BOOL("is_mac68k", CAIS_IS_MAC68K);
    SET_BOOL("is_x68k", CAIS_IS_X68K);
    SET_BOOL("is_wasi", CAIS_IS_WASI);

    return 1;
}
