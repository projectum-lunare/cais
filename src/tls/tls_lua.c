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

#include <cais/cais.h>
#include <cais/tls.h>
#include <cais/socket.h>

#define TLS_SESSION_MT "cais.tls.session"
#define SOCKET_MT      "cais.socket"

#if LUA_VERSION_NUM < 504
#define lua_newuserdatauv(L, sz, nuv) ((void)(nuv), lua_newuserdata(L, sz))
#endif

typedef struct {
    cais_tls_t *handle;
} lcais_tls_t;

typedef struct {
    cais_socket_t *handle;
} lcais_sock_ref_t;

static lcais_tls_t *check_tls_session(lua_State *L, int index)
{
    lcais_tls_t *t = (lcais_tls_t *)luaL_checkudata(L, index, TLS_SESSION_MT);
    if (!t || !t->handle) {
        luaL_error(L, "attempt to use closed or uninitialized cais.tls session");
    }
    return t;
}

static int lcais_tls_is_supported(lua_State *L)
{
    lua_pushboolean(L, cais_tls_is_supported());
    return 1;
}

static int lcais_tls_backend(lua_State *L)
{
    lua_pushstring(L, cais_tls_backend_name());
    return 1;
}

static int lcais_tls_create_internal(lua_State *L, int opt_idx, int sock_idx)
{
    cais_tls_config_t conf;
    cais_tls_t *tls;
    lcais_tls_t *t;
    char err[128];
    const char *sni = NULL;
    int verify = 1;
    cais_tls_role_t role = CAIS_TLS_CLIENT;

    if (opt_idx > 0 && lua_istable(L, opt_idx)) {
        lua_getfield(L, opt_idx, "server_name");
        if (lua_isstring(L, -1)) sni = lua_tostring(L, -1);
        else {
            lua_pop(L, 1);
            lua_getfield(L, opt_idx, "sni");
            if (lua_isstring(L, -1)) sni = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, opt_idx, "verify");
        if (lua_isboolean(L, -1)) verify = lua_toboolean(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, opt_idx, "role");
        if (lua_isinteger(L, -1)) role = (cais_tls_role_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
    }

    cais_tls_config_init(&conf, role);
    conf.server_name = sni;
    conf.verify = verify ? CAIS_TLS_VERIFY_REQUIRED : CAIS_TLS_VERIFY_NONE;

    tls = (cais_tls_t *)calloc(1, sizeof(cais_tls_t));
    if (!tls) {
        lua_pushnil(L);
        lua_pushstring(L, "out of memory");
        return 2;
    }

    if (cais_tls_init(tls, &conf, err, sizeof(err)) != CAIS_TLS_OK) {
        free(tls);
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "TLS initialization failed");
        return 2;
    }

    /* Allocate userdata */
    t = (lcais_tls_t *)lua_newuserdatauv(L, sizeof(lcais_tls_t), 1);
    t->handle = tls;
    luaL_getmetatable(L, TLS_SESSION_MT);
    lua_setmetatable(L, -2);

    /* If a socket was passed, bind it and anchor it in uservalue */
    if (sock_idx > 0 && !lua_isnil(L, sock_idx)) {
        lcais_sock_ref_t *s = (lcais_sock_ref_t *)luaL_testudata(L, sock_idx, SOCKET_MT);
        if (!s || !s->handle) {
            cais_tls_free(tls);
            free(tls);
            t->handle = NULL;
            lua_pushnil(L);
            lua_pushstring(L, "invalid cais.socket passed to tls");
            return 2;
        }

        cais_tls_set_socket(tls, s->handle);

        /* Anchor socket in uservalue slot 1 */
        lua_pushvalue(L, sock_idx);
#if LUA_VERSION_NUM >= 504
        lua_setiuservalue(L, -2, 1);
#else
        lua_setuservalue(L, -2);
#endif
    }

    return 1;
}

static int lcais_tls_create(lua_State *L)
{
    return lcais_tls_create_internal(L, 1, 0);
}

static int lcais_tls_wrap(lua_State *L)
{
    luaL_checkudata(L, 1, SOCKET_MT);
    return lcais_tls_create_internal(L, 2, 1);
}

static int lcais_tls_set_socket(lua_State *L)
{
    lcais_tls_t *t = check_tls_session(L, 1);
    lcais_sock_ref_t *s = (lcais_sock_ref_t *)luaL_checkudata(L, 2, SOCKET_MT);

    if (!s || !s->handle) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "invalid socket handle");
        return 2;
    }

    if (cais_tls_set_socket(t->handle, s->handle) != CAIS_TLS_OK) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "failed to attach socket to TLS session");
        return 2;
    }

    lua_pushvalue(L, 2);
#if LUA_VERSION_NUM >= 504
    lua_setiuservalue(L, 1, 1);
#else
    lua_setuservalue(L, 1);
#endif

    lua_pushboolean(L, 1);
    return 1;
}

static int lcais_tls_handshake(lua_State *L)
{
    lcais_tls_t *t = check_tls_session(L, 1);
    char err[128];
    int res = cais_tls_handshake(t->handle, err, sizeof(err));

    if (res == CAIS_TLS_OK) {
        lua_pushboolean(L, 1);
        return 1;
    }
    if (res == CAIS_TLS_WANT_READ) {
        lua_pushnil(L);
        lua_pushliteral(L, "want_read");
        return 2;
    }
    if (res == CAIS_TLS_WANT_WRITE) {
        lua_pushnil(L);
        lua_pushliteral(L, "want_write");
        return 2;
    }

    lua_pushnil(L);
    lua_pushstring(L, err[0] ? err : "TLS handshake failed");
    return 2;
}

static int lcais_tls_send(lua_State *L)
{
    lcais_tls_t *t = check_tls_session(L, 1);
    size_t len = 0;
    const char *data = luaL_checklstring(L, 2, &len);
    size_t written = 0;
    char err[128];
    int res;

    res = cais_tls_send(t->handle, data, len, &written, err, sizeof(err));
    if (res == CAIS_TLS_OK) {
        lua_pushinteger(L, (lua_Integer)written);
        return 1;
    }
    if (res == CAIS_TLS_WANT_WRITE) {
        lua_pushnil(L);
        lua_pushliteral(L, "want_write");
        return 2;
    }
    if (res == CAIS_TLS_WANT_READ) {
        lua_pushnil(L);
        lua_pushliteral(L, "want_read");
        return 2;
    }
    if (res == CAIS_TLS_CLOSED) {
        lua_pushnil(L);
        lua_pushliteral(L, "closed");
        return 2;
    }

    lua_pushnil(L);
    lua_pushstring(L, err[0] ? err : "TLS send failed");
    return 2;
}

static int lcais_tls_recv(lua_State *L)
{
    lcais_tls_t *t = check_tls_session(L, 1);
    lua_Integer max_len = luaL_optinteger(L, 2, 4096);
    char *buf;
    size_t read_bytes = 0;
    char err[128];
    int res;

    if (max_len <= 0) max_len = 4096;
    buf = (char *)malloc((size_t)max_len);
    if (!buf) {
        lua_pushnil(L);
        lua_pushstring(L, "out of memory");
        return 2;
    }

    res = cais_tls_recv(t->handle, buf, (size_t)max_len, &read_bytes, err, sizeof(err));
    if (res == CAIS_TLS_OK) {
        lua_pushlstring(L, buf, read_bytes);
        free(buf);
        return 1;
    }
    free(buf);

    if (res == CAIS_TLS_WANT_READ) {
        lua_pushnil(L);
        lua_pushliteral(L, "want_read");
        return 2;
    }
    if (res == CAIS_TLS_WANT_WRITE) {
        lua_pushnil(L);
        lua_pushliteral(L, "want_write");
        return 2;
    }
    if (res == CAIS_TLS_CLOSED) {
        lua_pushnil(L);
        lua_pushliteral(L, "closed");
        return 2;
    }

    lua_pushnil(L);
    lua_pushstring(L, err[0] ? err : "TLS recv failed");
    return 2;
}

static int lcais_tls_close(lua_State *L)
{
    lcais_tls_t *t = (lcais_tls_t *)luaL_checkudata(L, 1, TLS_SESSION_MT);
    if (t && t->handle) {
        cais_tls_close(t->handle);
        cais_tls_free(t->handle);
        free(t->handle);
        t->handle = NULL;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int lcais_tls_gc(lua_State *L)
{
    return lcais_tls_close(L);
}

static const struct luaL_Reg session_methods[] = {
    {"set_socket", lcais_tls_set_socket},
    {"handshake",  lcais_tls_handshake},
    {"send",       lcais_tls_send},
    {"write",      lcais_tls_send},
    {"recv",       lcais_tls_recv},
    {"read",       lcais_tls_recv},
    {"close",      lcais_tls_close},
    {"__gc",       lcais_tls_gc},
    {NULL, NULL}
};

static const struct luaL_Reg tls_funcs[] = {
    {"is_supported", lcais_tls_is_supported},
    {"backend",      lcais_tls_backend},
    {"create",       lcais_tls_create},
    {"wrap",         lcais_tls_wrap},
    {NULL, NULL}
};

int luaopen_cais_tls(lua_State *L);

int luaopen_cais_tls(lua_State *L)
{
    /* Metatable for TLS Session userdata */
    luaL_newmetatable(L, TLS_SESSION_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, session_methods, 0);
    lua_pop(L, 1);

    /* Module table */
    lua_newtable(L);
    luaL_setfuncs(L, tls_funcs, 0);

    /* Role enum */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_TLS_CLIENT); lua_setfield(L, -2, "CLIENT");
    lua_pushinteger(L, CAIS_TLS_SERVER); lua_setfield(L, -2, "SERVER");
    lua_setfield(L, -2, "role");

    /* Status enum */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_TLS_OK);         lua_setfield(L, -2, "OK");
    lua_pushinteger(L, CAIS_TLS_ERROR);      lua_setfield(L, -2, "ERROR");
    lua_pushinteger(L, CAIS_TLS_WANT_READ);  lua_setfield(L, -2, "WANT_READ");
    lua_pushinteger(L, CAIS_TLS_WANT_WRITE); lua_setfield(L, -2, "WANT_WRITE");
    lua_pushinteger(L, CAIS_TLS_CLOSED);     lua_setfield(L, -2, "CLOSED");
    lua_setfield(L, -2, "status");

    return 1;
}
