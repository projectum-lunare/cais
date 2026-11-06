/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cais/parallel.h>

#define PARALLEL_PORT_MT "cais.parallel.port"

typedef struct {
    cais_parallel_t *handle;
} lcais_parallel_t;

static int lcais_parallel_write(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    size_t written = 0;
    cais_status_t st;

    if (p->handle == NULL) {
        return luaL_error(L, "attempt to write to closed parallel port");
    }

    st = cais_parallel_write(p->handle, data, len, &written);
    if (st != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, "I/O error during parallel write");
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)written);
    return 1;
}

static int lcais_parallel_drain(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    if (p->handle == NULL) {
        return luaL_error(L, "attempt to drain closed parallel port");
    }

    if (cais_parallel_drain(p->handle) != CAIS_PARALLEL_OK) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "parallel drain failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lcais_parallel_read(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    lua_Integer max_bytes = luaL_optinteger(L, 2, 4096);
    char *buf;
    size_t read_bytes = 0;
    cais_status_t st;

    if (p->handle == NULL) {
        return luaL_error(L, "attempt to read from closed parallel port");
    }
    if (max_bytes <= 0) {
        lua_pushliteral(L, "");
        return 1;
    }

    buf = (char *)malloc((size_t)max_bytes);
    if (buf == NULL) {
        return luaL_error(L, "out of memory allocating parallel read buffer");
    }

    st = cais_parallel_read(p->handle, buf, (size_t)max_bytes, &read_bytes);
    if (st != CAIS_OK) {
        free(buf);
        lua_pushnil(L);
        lua_pushliteral(L, "I/O error or unsupported read on parallel port");
        return 2;
    }

    lua_pushlstring(L, buf, read_bytes);
    free(buf);
    return 1;
}

static int lcais_parallel_status(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    int pins = 0;
    cais_parallel_status_t st;

    if (p->handle == NULL) {
        return luaL_error(L, "attempt to query status of closed parallel port");
    }

    st = cais_parallel_get_status(p->handle, &pins);
    if (st != CAIS_PARALLEL_OK) {
        lua_pushnil(L);
        lua_pushliteral(L, "hardware status query unsupported or failed");
        lua_pushinteger(L, st);
        return 3;
    }

    lua_pushinteger(L, pins);
    return 1;
}

static int lcais_parallel_is_busy(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    int pins = 0;
    if (p->handle == NULL) return luaL_error(L, "attempt to query status of closed parallel port");
    if (cais_parallel_get_status(p->handle, &pins) != CAIS_PARALLEL_OK) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, (pins & CAIS_PARALLEL_PIN_BUSY) != 0);
    return 1;
}

static int lcais_parallel_is_ack(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    int pins = 0;
    if (p->handle == NULL) return luaL_error(L, "attempt to query status of closed parallel port");
    if (cais_parallel_get_status(p->handle, &pins) != CAIS_PARALLEL_OK) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, (pins & CAIS_PARALLEL_PIN_ACK) != 0);
    return 1;
}

static int lcais_parallel_is_pe(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    int pins = 0;
    if (p->handle == NULL) return luaL_error(L, "attempt to query status of closed parallel port");
    if (cais_parallel_get_status(p->handle, &pins) != CAIS_PARALLEL_OK) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, (pins & CAIS_PARALLEL_PIN_PE) != 0);
    return 1;
}

static int lcais_parallel_is_online(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    int pins = 0;
    if (p->handle == NULL) return luaL_error(L, "attempt to query status of closed parallel port");
    if (cais_parallel_get_status(p->handle, &pins) != CAIS_PARALLEL_OK) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, (pins & CAIS_PARALLEL_PIN_SLCT) != 0);
    return 1;
}

static int lcais_parallel_is_fault(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    int pins = 0;
    if (p->handle == NULL) return luaL_error(L, "attempt to query status of closed parallel port");
    if (cais_parallel_get_status(p->handle, &pins) != CAIS_PARALLEL_OK) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, (pins & CAIS_PARALLEL_PIN_FAULT) != 0);
    return 1;
}

static int lcais_parallel_set_mode(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    int mode = (int)luaL_checkinteger(L, 2);
    cais_parallel_status_t st;

    if (p->handle == NULL) {
        return luaL_error(L, "attempt to set mode on closed parallel port");
    }

    st = cais_parallel_set_mode(p->handle, (cais_parallel_mode_t)mode);
    if (st != CAIS_PARALLEL_OK) {
        lua_pushboolean(L, 0);
        lua_pushliteral(L, "mode selection unsupported on this device");
        lua_pushinteger(L, st);
        return 3;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lcais_parallel_close(lua_State *L)
{
    lcais_parallel_t *p = (lcais_parallel_t *)luaL_checkudata(L, 1, PARALLEL_PORT_MT);
    if (p->handle != NULL) {
        cais_parallel_close(p->handle);
        p->handle = NULL;
    }
    return 0;
}

static int lcais_parallel_open(lua_State *L)
{
    const char *path = "LPT1";
    char err_buf[256];
    cais_parallel_t *pp;
    lcais_parallel_t *p;

    err_buf[0] = '\0';
    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "path");
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_getfield(L, 1, "port");
        }
        if (lua_isstring(L, -1)) {
            path = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    } else if (!lua_isnoneornil(L, 1)) {
        path = luaL_checkstring(L, 1);
    }

    pp = cais_parallel_open(path, err_buf, sizeof(err_buf));
    if (pp == NULL) {
        lua_pushnil(L);
        if (err_buf[0] != '\0') {
            lua_pushstring(L, err_buf);
        } else {
            lua_pushstring(L, "failed to open parallel device");
        }
        return 2;
    }

    p = (lcais_parallel_t *)lua_newuserdatauv(L, sizeof(lcais_parallel_t), 0);
    p->handle = pp;

    luaL_getmetatable(L, PARALLEL_PORT_MT);
    lua_setmetatable(L, -2);

    return 1;
}

static const luaL_Reg port_methods[] = {
    {"write",         lcais_parallel_write       },
    {"read",          lcais_parallel_read        },
    {"status",        lcais_parallel_status      },
    {"get_status",    lcais_parallel_status      },
    {"is_busy",       lcais_parallel_is_busy     },
    {"is_ack",        lcais_parallel_is_ack      },
    {"is_pe",         lcais_parallel_is_pe       },
    {"is_paper_out",  lcais_parallel_is_pe       },
    {"is_online",     lcais_parallel_is_online   },
    {"is_slct",       lcais_parallel_is_online   },
    {"is_fault",      lcais_parallel_is_fault    },
    {"set_mode",      lcais_parallel_set_mode    },
    {"drain",         lcais_parallel_drain       },
    {"flush",         lcais_parallel_drain       },
    {"close",         lcais_parallel_close       },
    {"__close",       lcais_parallel_close       },
    {"__gc",          lcais_parallel_close       },
    {NULL,            NULL                       }
};

static const luaL_Reg parallel_funcs[] = {
    {"open", lcais_parallel_open},
    {NULL,   NULL          }
};

static void register_parallel_enums(lua_State *L)
{
    /* status */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_PARALLEL_OK);
    lua_setfield(L, -2, "OK");
    lua_pushinteger(L, CAIS_PARALLEL_ERROR);
    lua_setfield(L, -2, "ERROR");
    lua_pushinteger(L, CAIS_PARALLEL_UNSUPPORTED);
    lua_setfield(L, -2, "UNSUPPORTED");
    lua_setfield(L, -2, "status");

    /* mode */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_PARALLEL_MODE_SPP);
    lua_setfield(L, -2, "SPP");
    lua_pushinteger(L, CAIS_PARALLEL_MODE_SPP);
    lua_setfield(L, -2, "CENTRONICS");
    lua_pushinteger(L, CAIS_PARALLEL_MODE_NIBBLE);
    lua_setfield(L, -2, "NIBBLE");
    lua_pushinteger(L, CAIS_PARALLEL_MODE_BYTE);
    lua_setfield(L, -2, "BYTE");
    lua_pushinteger(L, CAIS_PARALLEL_MODE_EPP);
    lua_setfield(L, -2, "EPP");
    lua_pushinteger(L, CAIS_PARALLEL_MODE_ECP);
    lua_setfield(L, -2, "ECP");
    lua_setfield(L, -2, "mode");

    /* pin */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_PARALLEL_PIN_BUSY);
    lua_setfield(L, -2, "BUSY");
    lua_pushinteger(L, CAIS_PARALLEL_PIN_ACK);
    lua_setfield(L, -2, "ACK");
    lua_pushinteger(L, CAIS_PARALLEL_PIN_PE);
    lua_setfield(L, -2, "PE");
    lua_pushinteger(L, CAIS_PARALLEL_PIN_PE);
    lua_setfield(L, -2, "PAPER_OUT");
    lua_pushinteger(L, CAIS_PARALLEL_PIN_SLCT);
    lua_setfield(L, -2, "SLCT");
    lua_pushinteger(L, CAIS_PARALLEL_PIN_SLCT);
    lua_setfield(L, -2, "ONLINE");
    lua_pushinteger(L, CAIS_PARALLEL_PIN_FAULT);
    lua_setfield(L, -2, "FAULT");
    lua_setfield(L, -2, "pin");

    /* port */
    lua_newtable(L);
    lua_pushstring(L, "LPT1");
    lua_setfield(L, -2, "LPT1");
    lua_pushstring(L, "LPT2");
    lua_setfield(L, -2, "LPT2");
    lua_pushstring(L, "LPT3");
    lua_setfield(L, -2, "LPT3");
    lua_setfield(L, -2, "port");
}

int luaopen_cais_parallel(lua_State *L)
{
    if (luaL_newmetatable(L, PARALLEL_PORT_MT)) {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_setfuncs(L, port_methods, 0);
    }
    lua_pop(L, 1);

    luaL_newlib(L, parallel_funcs);
    register_parallel_enums(L);
    return 1;
}
