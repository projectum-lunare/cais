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
#include <cais/noise.h>

#define NOISE_SESSION_MT "cais.noise.session"

typedef struct {
    cais_noise_t *handle;
} lcais_noise_t;

static int lcais_noise_keygen(lua_State *L)
{
    unsigned char priv[CAIS_NOISE_KEY_LEN];
    unsigned char pub[CAIS_NOISE_KEY_LEN];
    char err[128];

    if (cais_noise_keygen(priv, pub, err, sizeof(err)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "Noise keygen failed");
        return 2;
    }

    lua_pushlstring(L, (const char *)priv, CAIS_NOISE_KEY_LEN);
    lua_pushlstring(L, (const char *)pub, CAIS_NOISE_KEY_LEN);
    return 2;
}

static int lcais_noise_seed(lua_State *L)
{
    size_t len;
    const char *entropy = luaL_checklstring(L, 1, &len);

    if (cais_noise_seed(entropy, len) != CAIS_OK) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int lcais_noise_create(lua_State *L)
{
    cais_noise_pattern_t pattern = CAIS_NOISE_XX;
    cais_noise_role_t role = CAIS_NOISE_INITIATOR;
    char err[128];
    lcais_noise_t *n;
    cais_noise_t *handle;

    if (lua_istable(L, 1)) {
        /* Pattern option */
        lua_getfield(L, 1, "pattern");
        if (lua_isstring(L, -1)) {
            const char *p = lua_tostring(L, -1);
            if (strcmp(p, "XX") == 0 || strcmp(p, "xx") == 0) pattern = CAIS_NOISE_XX;
            else if (strcmp(p, "NN") == 0 || strcmp(p, "nn") == 0) pattern = CAIS_NOISE_NN;
            else if (strcmp(p, "NK") == 0 || strcmp(p, "nk") == 0) pattern = CAIS_NOISE_NK;
            else if (strcmp(p, "IK") == 0 || strcmp(p, "ik") == 0) pattern = CAIS_NOISE_IK;
            else {
                lua_pushnil(L);
                lua_pushfstring(L, "unknown or unsupported Noise pattern '%s'", p);
                return 2;
            }
        } else if (lua_isinteger(L, -1)) {
            lua_Integer p_int = lua_tointeger(L, -1);
            if (p_int < 0 || p_int > 3) {
                lua_pushnil(L);
                lua_pushliteral(L, "invalid Noise pattern identifier");
                return 2;
            }
            pattern = (cais_noise_pattern_t)p_int;
        } else if (!lua_isnil(L, -1)) {
            lua_pushnil(L);
            lua_pushliteral(L, "invalid pattern option (expected string or integer)");
            return 2;
        }
        lua_pop(L, 1);

        /* Role option */
        lua_getfield(L, 1, "role");
        if (lua_isstring(L, -1)) {
            const char *r = lua_tostring(L, -1);
            if (strcmp(r, "initiator") == 0 || strcmp(r, "INITIATOR") == 0) role = CAIS_NOISE_INITIATOR;
            else if (strcmp(r, "responder") == 0 || strcmp(r, "RESPONDER") == 0) role = CAIS_NOISE_RESPONDER;
            else {
                lua_pushnil(L);
                lua_pushfstring(L, "invalid Noise role '%s' (must be 'initiator' or 'responder')", r);
                return 2;
            }
        } else if (lua_isinteger(L, -1)) {
            lua_Integer r_int = lua_tointeger(L, -1);
            if (r_int < 0 || r_int > 1) {
                lua_pushnil(L);
                lua_pushliteral(L, "invalid Noise role identifier");
                return 2;
            }
            role = (cais_noise_role_t)r_int;
        } else if (!lua_isnil(L, -1)) {
            lua_pushnil(L);
            lua_pushliteral(L, "invalid role option (expected string or integer)");
            return 2;
        }
        lua_pop(L, 1);
    }

    handle = cais_noise_create(pattern, role, err, sizeof(err));
    if (!handle) {
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "failed to create Noise session");
        return 2;
    }

    if (lua_istable(L, 1)) {
        /* Private key configuration */
        lua_getfield(L, 1, "private_key");
        if (!lua_isnil(L, -1)) {
            size_t klen = 0;
            const char *k;
            if (lua_type(L, -1) != LUA_TSTRING) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushliteral(L, "private_key must be a 32-byte binary string");
                return 2;
            }
            k = lua_tolstring(L, -1, &klen);
            if (klen != CAIS_NOISE_KEY_LEN) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushfstring(L, "private_key must be exactly 32 bytes (got %d)", (int)klen);
                return 2;
            }
            if (cais_noise_set_static_key(handle, (const unsigned char *)k, err, sizeof(err)) != CAIS_OK) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushstring(L, err[0] ? err : "failed to set private key");
                return 2;
            }
        }
        lua_pop(L, 1);

        /* Remote key configuration */
        lua_getfield(L, 1, "remote_key");
        if (!lua_isnil(L, -1)) {
            size_t klen = 0;
            const char *k;
            if (lua_type(L, -1) != LUA_TSTRING) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushliteral(L, "remote_key must be a 32-byte binary string");
                return 2;
            }
            k = lua_tolstring(L, -1, &klen);
            if (klen != CAIS_NOISE_KEY_LEN) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushfstring(L, "remote_key must be exactly 32 bytes (got %d)", (int)klen);
                return 2;
            }
            if (cais_noise_set_remote_key(handle, (const unsigned char *)k, err, sizeof(err)) != CAIS_OK) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushstring(L, err[0] ? err : "failed to set remote public key");
                return 2;
            }
        }
        lua_pop(L, 1);

        /* Ephemeral key configuration (testing / test vectors) */
        lua_getfield(L, 1, "ephemeral_key");
        if (!lua_isnil(L, -1)) {
            size_t klen = 0;
            const char *k;
            if (lua_type(L, -1) != LUA_TSTRING) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushliteral(L, "ephemeral_key must be a 32-byte binary string");
                return 2;
            }
            k = lua_tolstring(L, -1, &klen);
            if (klen != CAIS_NOISE_KEY_LEN) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushfstring(L, "ephemeral_key must be exactly 32 bytes (got %d)", (int)klen);
                return 2;
            }
            if (cais_noise_set_ephemeral_key(handle, (const unsigned char *)k, err, sizeof(err)) != CAIS_OK) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushstring(L, err[0] ? err : "failed to set ephemeral key");
                return 2;
            }
        }
        lua_pop(L, 1);

        /* Optional prologue */
        lua_getfield(L, 1, "prologue");
        if (!lua_isnil(L, -1)) {
            size_t plen = 0;
            const char *p;
            if (lua_type(L, -1) != LUA_TSTRING) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushliteral(L, "prologue must be a string");
                return 2;
            }
            p = lua_tolstring(L, -1, &plen);
            if (cais_noise_set_prologue(handle, p, plen, err, sizeof(err)) != CAIS_OK) {
                cais_noise_free(handle);
                lua_pushnil(L);
                lua_pushstring(L, err[0] ? err : "failed to set prologue");
                return 2;
            }
        }
        lua_pop(L, 1);
    }

    /* Start session state machine */
    if (cais_noise_start(handle, err, sizeof(err)) != CAIS_OK) {
        cais_noise_free(handle);
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "failed to start Noise state machine");
        return 2;
    }

    n = (lcais_noise_t *)lua_newuserdata(L, sizeof(lcais_noise_t));
    n->handle = handle;
    luaL_getmetatable(L, NOISE_SESSION_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int lcais_noise_session_action(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    cais_noise_action_t act;

    if (!n->handle) {
        return luaL_error(L, "attempt to query closed Noise session");
    }

    act = cais_noise_get_action(n->handle);
    switch (act) {
    case CAIS_NOISE_ACTION_WRITE_MSG:
        lua_pushliteral(L, "write_msg");
        break;
    case CAIS_NOISE_ACTION_READ_MSG:
        lua_pushliteral(L, "read_msg");
        break;
    case CAIS_NOISE_ACTION_SPLIT:
        lua_pushliteral(L, "split");
        break;
    default:
        lua_pushliteral(L, "failed");
        break;
    }
    return 1;
}

static int lcais_noise_session_is_complete(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    if (!n->handle) {
        return luaL_error(L, "attempt to query closed Noise session");
    }
    lua_pushboolean(L, cais_noise_is_complete(n->handle) ? 1 : 0);
    return 1;
}

static int lcais_noise_session_remote_static(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    const unsigned char *rs;

    if (!n->handle) {
        return luaL_error(L, "attempt to query closed Noise session");
    }

    rs = cais_noise_get_remote_static(n->handle);
    if (!rs) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlstring(L, (const char *)rs, CAIS_NOISE_KEY_LEN);
    return 1;
}

static int lcais_noise_session_handshake_hash(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    unsigned char hash[CAIS_NOISE_HASH_LEN];
    char err[128];

    if (!n->handle) {
        return luaL_error(L, "attempt to query closed Noise session");
    }

    if (cais_noise_get_handshake_hash(n->handle, hash, err, sizeof(err)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "failed to get handshake hash");
        return 2;
    }

    lua_pushlstring(L, (const char *)hash, CAIS_NOISE_HASH_LEN);
    return 1;
}

static int lcais_noise_session_write_message(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    size_t payload_len = 0;
    const char *payload = NULL;
    char err[128];
    unsigned char out_buf[256];
    size_t out_len = 0;

    if (!n->handle) {
        return luaL_error(L, "attempt to use closed Noise session");
    }

    if (!lua_isnoneornil(L, 2)) {
        payload = luaL_checklstring(L, 2, &payload_len);
    }

    if (cais_noise_write_message(n->handle, payload, payload_len, out_buf, sizeof(out_buf), &out_len, err, sizeof(err)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "Noise write message failed");
        return 2;
    }

    lua_pushlstring(L, (const char *)out_buf, out_len);
    return 1;
}

static int lcais_noise_session_read_message(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    size_t msg_len = 0;
    const char *msg = luaL_checklstring(L, 2, &msg_len);
    char err[128];
    unsigned char payload_buf[256];
    size_t payload_len = 0;

    if (!n->handle) {
        return luaL_error(L, "attempt to use closed Noise session");
    }

    if (cais_noise_read_message(n->handle, msg, msg_len, payload_buf, sizeof(payload_buf), &payload_len, err, sizeof(err)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "Noise read message failed");
        return 2;
    }

    lua_pushlstring(L, (const char *)payload_buf, payload_len);
    return 1;
}

static int lcais_noise_session_encrypt(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    size_t plain_len = 0;
    const char *plain = luaL_checklstring(L, 2, &plain_len);
    char err[128];
    size_t out_len = 0;
    unsigned char *cipher;

    if (!n->handle) {
        return luaL_error(L, "attempt to use closed Noise session");
    }

    cipher = (unsigned char *)malloc(plain_len + 16);
    if (!cipher) {
        return luaL_error(L, "out of memory allocating encryption buffer");
    }

    if (cais_noise_encrypt(n->handle, plain, plain_len, cipher, plain_len + 16, &out_len, err, sizeof(err)) != CAIS_OK) {
        free(cipher);
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "Noise encryption failed");
        return 2;
    }

    lua_pushlstring(L, (const char *)cipher, out_len);
    free(cipher);
    return 1;
}

static int lcais_noise_session_decrypt(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    size_t cipher_len = 0;
    const char *cipher = luaL_checklstring(L, 2, &cipher_len);
    char err[128];
    size_t out_len = 0;
    unsigned char *plain;

    if (!n->handle) {
        return luaL_error(L, "attempt to use closed Noise session");
    }

    if (cipher_len < 16) {
        lua_pushnil(L);
        lua_pushstring(L, "ciphertext too short (must contain at least 16-byte MAC tag)");
        return 2;
    }

    plain = (unsigned char *)malloc(cipher_len - 16);
    if (!plain) {
        return luaL_error(L, "out of memory allocating decryption buffer");
    }

    if (cais_noise_decrypt(n->handle, cipher, cipher_len, plain, cipher_len - 16, &out_len, err, sizeof(err)) != CAIS_OK) {
        free(plain);
        lua_pushnil(L);
        lua_pushstring(L, err[0] ? err : "Noise decryption failed (authentication tag mismatch)");
        return 2;
    }

    lua_pushlstring(L, (const char *)plain, out_len);
    free(plain);
    return 1;
}

static int lcais_noise_session_close(lua_State *L)
{
    lcais_noise_t *n = (lcais_noise_t *)luaL_checkudata(L, 1, NOISE_SESSION_MT);
    if (n->handle) {
        cais_noise_free(n->handle);
        n->handle = NULL;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static const struct luaL_Reg session_methods[] = {
    {"action",         lcais_noise_session_action},
    {"is_complete",    lcais_noise_session_is_complete},
    {"remote_static",  lcais_noise_session_remote_static},
    {"handshake_hash", lcais_noise_session_handshake_hash},
    {"write_message",  lcais_noise_session_write_message},
    {"read_message",   lcais_noise_session_read_message},
    {"encrypt",        lcais_noise_session_encrypt},
    {"decrypt",        lcais_noise_session_decrypt},
    {"close",          lcais_noise_session_close},
    {"__gc",           lcais_noise_session_close},
    {NULL, NULL}
};

static const struct luaL_Reg noise_funcs[] = {
    {"keygen", lcais_noise_keygen},
    {"seed",   lcais_noise_seed},
    {"create", lcais_noise_create},
    {NULL, NULL}
};

int luaopen_cais_noise(lua_State *L);

int luaopen_cais_noise(lua_State *L)
{
    /* Metatable for Noise Session userdata */
    luaL_newmetatable(L, NOISE_SESSION_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, session_methods, 0);
    lua_pop(L, 1);

    /* cais.noise module table */
    lua_newtable(L);
    luaL_setfuncs(L, noise_funcs, 0);

    /* Pattern enum table */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_NOISE_XX); lua_setfield(L, -2, "XX");
    lua_pushinteger(L, CAIS_NOISE_NN); lua_setfield(L, -2, "NN");
    lua_pushinteger(L, CAIS_NOISE_NK); lua_setfield(L, -2, "NK");
    lua_pushinteger(L, CAIS_NOISE_IK); lua_setfield(L, -2, "IK");
    lua_setfield(L, -2, "pattern");

    /* Role enum table */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_NOISE_INITIATOR); lua_setfield(L, -2, "INITIATOR");
    lua_pushinteger(L, CAIS_NOISE_RESPONDER); lua_setfield(L, -2, "RESPONDER");
    lua_setfield(L, -2, "role");

    /* Action enum table */
    lua_newtable(L);
    lua_pushinteger(L, CAIS_NOISE_ACTION_WRITE_MSG); lua_setfield(L, -2, "WRITE_MSG");
    lua_pushinteger(L, CAIS_NOISE_ACTION_READ_MSG);  lua_setfield(L, -2, "READ_MSG");
    lua_pushinteger(L, CAIS_NOISE_ACTION_SPLIT);     lua_setfield(L, -2, "SPLIT");
    lua_pushinteger(L, CAIS_NOISE_ACTION_FAILED);    lua_setfield(L, -2, "FAILED");
    lua_setfield(L, -2, "action");

    return 1;
}
