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

#include <cais/sandbox.h>
#include "sandbox_internal.h"

#define SANDBOX_POLICY_MT "cais.sandbox.policy"

#if LUA_VERSION_NUM < 502
static void *cais_testudata(lua_State *L, int ud, const char *tname)
{
    void *p = lua_touserdata(L, ud);
    if (p != NULL) {
        if (lua_getmetatable(L, ud)) {
            luaL_getmetatable(L, tname);
            if (!lua_rawequal(L, -1, -2)) {
                p = NULL;
            }
            lua_pop(L, 2);
            return p;
        }
    }
    return NULL;
}
#define luaL_testudata cais_testudata
#endif

typedef struct {
    cais_sandbox_policy_t *policy;
} lcais_sandbox_policy_t;

static int lsandbox_backend(lua_State *L)
{
    int version = 0;
    cais_sandbox_backend_t id = cais_sandbox_backend(&version);
    const char *name = cais_sandbox_backend_name(NULL);
    lua_pushstring(L, name);
    lua_pushinteger(L, version);
    lua_pushinteger(L, (lua_Integer)id);
    return 3;
}

static int lsandbox_is_supported(lua_State *L)
{
    lua_pushboolean(L, cais_sandbox_is_supported() == CAIS_TRUE);
    return 1;
}

static int parse_perms(const char *p)
{
    int flags = 0;
    if (p == NULL)
        return CAIS_SANDBOX_PERM_READ;
    if (strchr(p, 'r') || strchr(p, 'R'))
        flags |= CAIS_SANDBOX_PERM_READ;
    if (strchr(p, 'w') || strchr(p, 'W'))
        flags |= CAIS_SANDBOX_PERM_WRITE;
    if (strchr(p, 'x') || strchr(p, 'X'))
        flags |= CAIS_SANDBOX_PERM_EXEC;
    return flags ? flags : CAIS_SANDBOX_PERM_READ;
}

static cais_sandbox_net_t parse_net(const char *s)
{
    if (s == NULL) return CAIS_SANDBOX_NET_DEFAULT;
    if (strcmp(s, "none") == 0) return CAIS_SANDBOX_NET_NONE;
    if (strcmp(s, "loopback") == 0) return CAIS_SANDBOX_NET_LOOPBACK;
    if (strcmp(s, "all") == 0) return CAIS_SANDBOX_NET_ALL;
    return CAIS_SANDBOX_NET_DEFAULT;
}

static void populate_policy_from_table(lua_State *L, int index, cais_sandbox_policy_t *policy)
{
    /* 1. Parse 'paths' */
    lua_getfield(L, index, "paths");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            const char *path = NULL;
            int perms = CAIS_SANDBOX_PERM_READ;

            if (lua_istable(L, -1)) {
                /* { path = "...", perm = "..." } */
                lua_getfield(L, -1, "path");
                if (lua_isstring(L, -1)) {
                    path = lua_tostring(L, -1);
                }
                lua_pop(L, 1);

                lua_getfield(L, -1, "perm");
                if (lua_isinteger(L, -1)) {
                    perms = (int)lua_tointeger(L, -1);
                } else if (lua_isstring(L, -1)) {
                    perms = parse_perms(lua_tostring(L, -1));
                }
                lua_pop(L, 1);
            } else if (lua_isstring(L, -2) && (lua_isstring(L, -1) || lua_isinteger(L, -1))) {
                /* Dictionary: [path] = "rw" */
                path = lua_tostring(L, -2);
                if (lua_isinteger(L, -1)) {
                    perms = (int)lua_tointeger(L, -1);
                } else {
                    perms = parse_perms(lua_tostring(L, -1));
                }
            } else if (lua_isstring(L, -1)) {
                /* Array of paths: { "/etc", "/tmp" } */
                path = lua_tostring(L, -1);
                perms = CAIS_SANDBOX_PERM_READ;
            }

            if (path != NULL && path[0] != '\0') {
                cais_sandbox_policy_allow_path(policy, path, perms);
            }

            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    /* 2. Parse 'network' */
    lua_getfield(L, index, "network");
    if (lua_isinteger(L, -1)) {
        cais_sandbox_policy_set_network(policy, (cais_sandbox_net_t)lua_tointeger(L, -1));
    } else if (lua_isstring(L, -1)) {
        cais_sandbox_policy_set_network(policy, parse_net(lua_tostring(L, -1)));
    } else if (lua_isboolean(L, -1)) {
        if (!lua_toboolean(L, -1)) {
            cais_sandbox_policy_set_network(policy, CAIS_SANDBOX_NET_NONE);
        } else {
            cais_sandbox_policy_set_network(policy, CAIS_SANDBOX_NET_ALL);
        }
    }
    lua_pop(L, 1);

    /* 3. Parse 'child_processes' */
    lua_getfield(L, index, "child_processes");
    if (lua_isboolean(L, -1)) {
        cais_sandbox_policy_set_allow_child_processes(
            policy, lua_toboolean(L, -1) ? CAIS_TRUE : CAIS_FALSE
        );
    }
    lua_pop(L, 1);

    /* 4. Parse 'dynamic_code' */
    lua_getfield(L, index, "dynamic_code");
    if (lua_isboolean(L, -1)) {
        cais_sandbox_policy_set_allow_dynamic_code(
            policy, lua_toboolean(L, -1) ? CAIS_TRUE : CAIS_FALSE
        );
    }
    lua_pop(L, 1);
}

/*
 * =============================================================================
 * OOP Policy Userdata Methods
 * =============================================================================
 */

static int lcais_policy_allow(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    const char *path = luaL_checkstring(L, 2);
    int perms = CAIS_SANDBOX_PERM_READ;

    if (p->policy == NULL) {
        return luaL_error(L, "attempt to operate on invalid sandbox policy");
    }

    if (lua_isinteger(L, 3)) {
        perms = (int)lua_tointeger(L, 3);
    } else if (lua_isstring(L, 3)) {
        perms = parse_perms(lua_tostring(L, 3));
    }

    cais_sandbox_policy_allow_path(p->policy, path, perms);
    lua_pushvalue(L, 1); /* return self for method chaining */
    return 1;
}

static int lcais_policy_set_network(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    cais_sandbox_net_t net = CAIS_SANDBOX_NET_DEFAULT;

    if (p->policy == NULL) {
        return luaL_error(L, "attempt to operate on invalid sandbox policy");
    }

    if (lua_isinteger(L, 2)) {
        net = (cais_sandbox_net_t)lua_tointeger(L, 2);
    } else if (lua_isstring(L, 2)) {
        net = parse_net(lua_tostring(L, 2));
    } else if (lua_isboolean(L, 2)) {
        net = lua_toboolean(L, 2) ? CAIS_SANDBOX_NET_ALL : CAIS_SANDBOX_NET_NONE;
    }

    cais_sandbox_policy_set_network(p->policy, net);
    lua_pushvalue(L, 1);
    return 1;
}

static int lcais_policy_deny_network(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    if (p->policy == NULL) {
        return luaL_error(L, "attempt to operate on invalid sandbox policy");
    }
    cais_sandbox_policy_set_network(p->policy, CAIS_SANDBOX_NET_NONE);
    lua_pushvalue(L, 1);
    return 1;
}

static int lcais_policy_set_child_processes(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    int allow;
    if (p->policy == NULL) {
        return luaL_error(L, "attempt to operate on invalid sandbox policy");
    }
    allow = lua_toboolean(L, 2);
    cais_sandbox_policy_set_allow_child_processes(p->policy, allow ? CAIS_TRUE : CAIS_FALSE);
    lua_pushvalue(L, 1);
    return 1;
}

static int lcais_policy_no_child_processes(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    if (p->policy == NULL) {
        return luaL_error(L, "attempt to operate on invalid sandbox policy");
    }
    cais_sandbox_policy_set_allow_child_processes(p->policy, CAIS_FALSE);
    lua_pushvalue(L, 1);
    return 1;
}

static int lcais_policy_set_dynamic_code(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    int allow;
    if (p->policy == NULL) {
        return luaL_error(L, "attempt to operate on invalid sandbox policy");
    }
    allow = lua_toboolean(L, 2);
    cais_sandbox_policy_set_allow_dynamic_code(p->policy, allow ? CAIS_TRUE : CAIS_FALSE);
    lua_pushvalue(L, 1);
    return 1;
}

static int lcais_policy_no_dynamic_code(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    if (p->policy == NULL) {
        return luaL_error(L, "attempt to operate on invalid sandbox policy");
    }
    cais_sandbox_policy_set_allow_dynamic_code(p->policy, CAIS_FALSE);
    lua_pushvalue(L, 1);
    return 1;
}

static int lcais_policy_apply(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    char err_buf[256];
    cais_sandbox_status_t res;

    if (p->policy == NULL) {
        return luaL_error(L, "attempt to apply invalid sandbox policy");
    }

    err_buf[0] = '\0';
    res = cais_sandbox_apply(p->policy, err_buf, sizeof(err_buf));
    if (res != CAIS_SANDBOX_OK) {
        lua_pushboolean(L, 0);
        if (err_buf[0] != '\0') {
            lua_pushstring(L, err_buf);
        } else {
            lua_pushliteral(L, "sandbox apply failed");
        }
        lua_pushinteger(L, (lua_Integer)res);
        return 3;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lcais_policy_gc(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    if (p->policy != NULL) {
        cais_sandbox_policy_destroy(p->policy);
        p->policy = NULL;
    }
    return 0;
}

static int lcais_policy_tostring(lua_State *L)
{
    lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_checkudata(L, 1, SANDBOX_POLICY_MT);
    char desc[64];
    size_t count = (p->policy != NULL) ? cais_sandbox_policy_path_count(p->policy) : 0;
    sprintf(desc, "cais.sandbox.policy (%lu paths)", (unsigned long)count);
    lua_pushstring(L, desc);
    return 1;
}

/*
 * Constructor: cais.sandbox.policy([options])
 */
static int lcais_sandbox_policy_new(lua_State *L)
{
    lcais_sandbox_policy_t *ud;
    cais_sandbox_policy_t *policy = cais_sandbox_policy_create();
    if (policy == NULL) {
        return luaL_error(L, "failed to allocate sandbox policy structure");
    }

    if (lua_istable(L, 1)) {
        populate_policy_from_table(L, 1, policy);
    }

    ud = (lcais_sandbox_policy_t *)lua_newuserdata(L, sizeof(lcais_sandbox_policy_t));
    ud->policy = policy;

    luaL_getmetatable(L, SANDBOX_POLICY_MT);
    lua_setmetatable(L, -2);
    return 1;
}

/*
 * Polymorphic cais.sandbox.apply([policy | options])
 */
static int lcais_sandbox_apply(lua_State *L)
{
    char err_buf[256];
    cais_sandbox_status_t res;
    cais_sandbox_policy_t *policy = NULL;
    int destroy_policy = 0;

    err_buf[0] = '\0';

    if (lua_isuserdata(L, 1)) {
        lcais_sandbox_policy_t *p = (lcais_sandbox_policy_t *)luaL_testudata(L, 1, SANDBOX_POLICY_MT);
        if (p != NULL) {
            policy = p->policy;
        } else {
            return luaL_argerror(L, 1, "expected cais.sandbox.policy userdata or options table");
        }
    } else if (lua_istable(L, 1)) {
        policy = cais_sandbox_policy_create();
        if (policy == NULL) {
            lua_pushboolean(L, 0);
            lua_pushliteral(L, "failed to allocate sandbox policy structure");
            lua_pushinteger(L, (lua_Integer)CAIS_SANDBOX_ERROR);
            return 3;
        }
        populate_policy_from_table(L, 1, policy);
        destroy_policy = 1;
    }

    res = cais_sandbox_apply(policy, err_buf, sizeof(err_buf));
    if (destroy_policy && policy != NULL) {
        cais_sandbox_policy_destroy(policy);
    }

    if (res != CAIS_SANDBOX_OK) {
        lua_pushboolean(L, 0);
        if (err_buf[0] != '\0') {
            lua_pushstring(L, err_buf);
        } else {
            lua_pushliteral(L, "sandbox apply failed");
        }
        lua_pushinteger(L, (lua_Integer)res);
        return 3;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static const luaL_Reg policy_methods[] = {
    {"allow",               lcais_policy_allow              },
    {"set_network",         lcais_policy_set_network        },
    {"network",             lcais_policy_set_network        },
    {"deny_network",        lcais_policy_deny_network       },
    {"set_child_processes", lcais_policy_set_child_processes},
    {"no_child_processes",  lcais_policy_no_child_processes },
    {"set_dynamic_code",    lcais_policy_set_dynamic_code   },
    {"no_dynamic_code",     lcais_policy_no_dynamic_code    },
    {"apply",               lcais_policy_apply              },
    {"__gc",                lcais_policy_gc                 },
    {"__tostring",          lcais_policy_tostring           },
    {NULL,                  NULL                            }
};

static const luaL_Reg sandbox_funcs[] = {
    {"backend",      lsandbox_backend         },
    {"get_backend",  lsandbox_backend         },
    {"is_supported", lsandbox_is_supported    },
    {"policy",       lcais_sandbox_policy_new },
    {"Policy",       lcais_sandbox_policy_new },
    {"apply",        lcais_sandbox_apply      },
    {NULL,           NULL                     }
};

static void register_policy_metatable(lua_State *L)
{
    luaL_newmetatable(L, SANDBOX_POLICY_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(L, policy_methods, 0);
#else
    luaL_register(L, NULL, policy_methods);
#endif
    lua_pop(L, 1);
}

static void register_enums(lua_State *L)
{
    /* cais.sandbox.backend (callable table with enum constants) */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_SANDBOX_BACKEND_NONE);     lua_setfield(L, -2, "NONE");
    lua_pushinteger(L, CAIS_SANDBOX_BACKEND_LANDLOCK); lua_setfield(L, -2, "LANDLOCK");
    lua_pushinteger(L, CAIS_SANDBOX_BACKEND_OPENBSD);  lua_setfield(L, -2, "OPENBSD");
    lua_pushinteger(L, CAIS_SANDBOX_BACKEND_CAPSICUM); lua_setfield(L, -2, "CAPSICUM");
    lua_pushinteger(L, CAIS_SANDBOX_BACKEND_SEATBELT); lua_setfield(L, -2, "SEATBELT");
    lua_pushinteger(L, CAIS_SANDBOX_BACKEND_WIN32);    lua_setfield(L, -2, "WIN32");

    lua_newtable(L);
    lua_pushcfunction(L, lsandbox_backend);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);

    /* Also provide cais.sandbox.backends alias */
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "backends");
    lua_setfield(L, -2, "backend");

    /* cais.sandbox.perm */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_SANDBOX_PERM_NONE);  lua_setfield(L, -2, "NONE");
    lua_pushinteger(L, CAIS_SANDBOX_PERM_READ);  lua_setfield(L, -2, "READ");
    lua_pushinteger(L, CAIS_SANDBOX_PERM_WRITE); lua_setfield(L, -2, "WRITE");
    lua_pushinteger(L, CAIS_SANDBOX_PERM_EXEC);  lua_setfield(L, -2, "EXEC");
    lua_setfield(L, -2, "perm");

    /* cais.sandbox.net */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_SANDBOX_NET_DEFAULT);  lua_setfield(L, -2, "DEFAULT");
    lua_pushinteger(L, CAIS_SANDBOX_NET_NONE);     lua_setfield(L, -2, "NONE");
    lua_pushinteger(L, CAIS_SANDBOX_NET_LOOPBACK); lua_setfield(L, -2, "LOOPBACK");
    lua_pushinteger(L, CAIS_SANDBOX_NET_ALL);      lua_setfield(L, -2, "ALL");
    lua_setfield(L, -2, "net");

    /* cais.sandbox.status */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_SANDBOX_OK);          lua_setfield(L, -2, "OK");
    lua_pushinteger(L, CAIS_SANDBOX_ERROR);       lua_setfield(L, -2, "ERROR");
    lua_pushinteger(L, CAIS_SANDBOX_UNSUPPORTED); lua_setfield(L, -2, "UNSUPPORTED");
    lua_setfield(L, -2, "status");

    /* Direct constants */
    lua_pushinteger(L, CAIS_SANDBOX_PERM_READ);   lua_setfield(L, -2, "READ");
    lua_pushinteger(L, CAIS_SANDBOX_PERM_WRITE);  lua_setfield(L, -2, "WRITE");
    lua_pushinteger(L, CAIS_SANDBOX_PERM_EXEC);   lua_setfield(L, -2, "EXEC");
    lua_pushinteger(L, CAIS_SANDBOX_OK);          lua_setfield(L, -2, "OK");
    lua_pushinteger(L, CAIS_SANDBOX_ERROR);       lua_setfield(L, -2, "ERROR");
    lua_pushinteger(L, CAIS_SANDBOX_UNSUPPORTED); lua_setfield(L, -2, "UNSUPPORTED");
}

int luaopen_cais_sandbox(lua_State *L);

int luaopen_cais_sandbox(lua_State *L)
{
    register_policy_metatable(L);
#if LUA_VERSION_NUM >= 502
    luaL_newlib(L, sandbox_funcs);
#else
    lua_newtable(L);
    luaL_register(L, NULL, sandbox_funcs);
#endif
    register_enums(L);
    return 1;
}
