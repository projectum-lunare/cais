/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
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

#include <cais/filesystem.h>

#if LUA_VERSION_NUM < 504
#define lua_newuserdatauv(L, sz, nuv) ((void)(nuv), lua_newuserdata(L, sz))
#endif

#define CAIS_FILE_MT "cais.filesystem.file"
#define CAIS_DIR_MT  "cais.filesystem.dir"

typedef struct {
    cais_file_t *handle;
} lcais_file_t;

typedef struct {
    cais_fs_dir_t *handle;
    int closed;
} lcais_dir_t;

static const char *fs_type_to_string(cais_fs_type_t t)
{
    switch (t) {
    case CAIS_FS_TYPE_FILE:    return "file";
    case CAIS_FS_TYPE_DIR:     return "dir";
    case CAIS_FS_TYPE_SYMLINK: return "symlink";
    case CAIS_FS_TYPE_DEVICE:  return "device";
    case CAIS_FS_TYPE_FIFO:    return "fifo";
    case CAIS_FS_TYPE_SOCKET:  return "socket";
    default:                   return "unknown";
    }
}

static void push_stat_table(lua_State *L, const cais_fs_stat_t *st)
{
    lua_createtable(L, 0, 7);

    lua_pushstring(L, fs_type_to_string(st->type));
    lua_setfield(L, -2, "type");

    lua_pushinteger(L, (lua_Integer)st->type);
    lua_setfield(L, -2, "type_code");

    lua_pushinteger(L, (lua_Integer)st->size);
    lua_setfield(L, -2, "size");

    lua_pushinteger(L, (lua_Integer)st->mtime);
    lua_setfield(L, -2, "mtime");

    lua_pushinteger(L, (lua_Integer)st->atime);
    lua_setfield(L, -2, "atime");

    lua_pushinteger(L, (lua_Integer)st->ctime);
    lua_setfield(L, -2, "ctime");

    lua_pushinteger(L, (lua_Integer)st->mode);
    lua_setfield(L, -2, "mode");

    lua_pushboolean(L, st->is_readonly == CAIS_TRUE);
    lua_setfield(L, -2, "is_readonly");
}

/* =============================================================================
 * Predicates & Metadata
 * ============================================================================= */

static int lfs_exists(lua_State *L)
{
    const char *path = luaL_optstring(L, 1, NULL);
    if (path == NULL) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, cais_fs_exists(path) == CAIS_TRUE);
    return 1;
}

static int lfs_stat(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char err_buf[128];
    cais_fs_stat_t st;
    err_buf[0] = '\0';

    if (cais_fs_stat(path, &st, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "stat failed");
        return 2;
    }

    push_stat_table(L, &st);
    return 1;
}

static int lfs_lstat(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char err_buf[128];
    cais_fs_stat_t st;
    err_buf[0] = '\0';

    if (cais_fs_lstat(path, &st, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "lstat failed");
        return 2;
    }

    push_stat_table(L, &st);
    return 1;
}

static int lfs_is_file(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, cais_fs_is_file(path) == CAIS_TRUE);
    return 1;
}

static int lfs_is_dir(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, cais_fs_is_dir(path) == CAIS_TRUE);
    return 1;
}

static int lfs_is_symlink(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, cais_fs_is_symlink(path) == CAIS_TRUE);
    return 1;
}

static int lfs_file_size(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    cais_fsize_t size = 0;
    if (cais_fs_file_size(path, &size) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushliteral(L, "failed to query file size");
        return 2;
    }
    lua_pushinteger(L, (lua_Integer)size);
    return 1;
}

/* =============================================================================
 * Whole-File Convenience (Slurp / Spit)
 * ============================================================================= */

static int lfs_read(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char err_buf[128];
    void *buf = NULL;
    size_t len = 0;
    err_buf[0] = '\0';

    if (cais_fs_read_file(path, &buf, &len, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "read failed");
        return 2;
    }

    lua_pushlstring(L, (const char *)buf, len);
    free(buf);
    return 1;
}

static int lfs_write(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    size_t len = 0;
    const char *data = luaL_checklstring(L, 2, &len);
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_write_file(path, data, len, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "write failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lfs_append(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    size_t len = 0;
    const char *data = luaL_checklstring(L, 2, &len);
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_append_file(path, data, len, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "append failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/* =============================================================================
 * Directory & Mutation Operations
 * ============================================================================= */

static int lfs_mkdir(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    int recursive = lua_toboolean(L, 2);
    int mode = (int)luaL_optinteger(L, 3, 0777);
    char err_buf[128];
    cais_status_t st;
    err_buf[0] = '\0';

    if (recursive) {
        st = cais_fs_mkdir_p(path, mode, err_buf, sizeof(err_buf));
    } else {
        st = cais_fs_mkdir(path, mode, err_buf, sizeof(err_buf));
    }

    if (st != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "mkdir failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lfs_rmdir(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_rmdir(path, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "rmdir failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lfs_remove(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    int recursive = lua_toboolean(L, 2);
    char err_buf[128];
    cais_status_t st;
    err_buf[0] = '\0';

    if (recursive) {
        st = cais_fs_remove_all(path, err_buf, sizeof(err_buf));
    } else {
        st = cais_fs_remove(path, err_buf, sizeof(err_buf));
    }

    if (st != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "remove failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lfs_rename(lua_State *L)
{
    const char *oldpath = luaL_checkstring(L, 1);
    const char *newpath = luaL_checkstring(L, 2);
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_rename(oldpath, newpath, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "rename failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lfs_copy(lua_State *L)
{
    const char *src = luaL_checkstring(L, 1);
    const char *dst = luaL_checkstring(L, 2);
    cais_bool_t overwrite = lua_isnoneornil(L, 3) ? CAIS_TRUE : (lua_toboolean(L, 3) ? CAIS_TRUE : CAIS_FALSE);
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_copy_file(src, dst, overwrite, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "copy failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lfs_chmod(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    int mode = (int)luaL_checkinteger(L, 2);
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_chmod(path, mode, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "chmod failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/* =============================================================================
 * Environment & Special Directories
 * ============================================================================= */

static int lfs_cwd(lua_State *L)
{
    char buf[1024];
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_getcwd(buf, sizeof(buf), err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "getcwd failed");
        return 2;
    }

    lua_pushstring(L, buf);
    return 1;
}

static int lfs_chdir(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_setcwd(path, err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "chdir failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lfs_temp_dir(lua_State *L)
{
    char buf[1024];
    char err_buf[128];
    err_buf[0] = '\0';

    if (cais_fs_temp_dir(buf, sizeof(buf), err_buf, sizeof(err_buf)) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "temp_dir failed");
        return 2;
    }

    lua_pushstring(L, buf);
    return 1;
}

/* =============================================================================
 * Directory Iterator & List
 * ============================================================================= */

static int dir_iter(lua_State *L)
{
    lcais_dir_t *d = (lcais_dir_t *)lua_touserdata(L, lua_upvalueindex(1));
    cais_fs_dirent_t ent;
    cais_status_t st;

    if (d == NULL || d->closed || d->handle == NULL) {
        return 0;
    }

    st = cais_fs_dir_next(d->handle, &ent);
    if (st != CAIS_OK) {
        cais_fs_dir_close(d->handle);
        d->handle = NULL;
        d->closed = 1;
        return 0;
    }

    lua_createtable(L, 0, 2);
    lua_pushstring(L, ent.name);
    lua_setfield(L, -2, "name");

    lua_pushstring(L, fs_type_to_string(ent.type));
    lua_setfield(L, -2, "type");

    return 1;
}

static int dir_gc(lua_State *L)
{
    lcais_dir_t *d = (lcais_dir_t *)luaL_testudata(L, 1, CAIS_DIR_MT);
    if (d != NULL && !d->closed && d->handle != NULL) {
        cais_fs_dir_close(d->handle);
        d->handle = NULL;
        d->closed = 1;
    }
    return 0;
}

static int lfs_dir(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char err_buf[128];
    cais_fs_dir_t *dir;
    lcais_dir_t *ud;
    err_buf[0] = '\0';

    dir = cais_fs_dir_open(path, err_buf, sizeof(err_buf));
    if (dir == NULL) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "failed to open directory");
        return 2;
    }

    ud = (lcais_dir_t *)lua_newuserdatauv(L, sizeof(lcais_dir_t), 0);
    ud->handle = dir;
    ud->closed = 0;

    luaL_getmetatable(L, CAIS_DIR_MT);
    lua_setmetatable(L, -2);

    lua_pushcclosure(L, dir_iter, 1);
    return 1;
}

static int lfs_list(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char err_buf[128];
    cais_fs_dir_t *dir;
    cais_fs_dirent_t ent;
    int idx = 1;
    err_buf[0] = '\0';

    dir = cais_fs_dir_open(path, err_buf, sizeof(err_buf));
    if (dir == NULL) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "failed to open directory");
        return 2;
    }

    lua_newtable(L);

    while (cais_fs_dir_next(dir, &ent) == CAIS_OK) {
        lua_createtable(L, 0, 2);
        lua_pushstring(L, ent.name);
        lua_setfield(L, -2, "name");
        lua_pushstring(L, fs_type_to_string(ent.type));
        lua_setfield(L, -2, "type");

        lua_rawseti(L, -2, idx++);
    }

    cais_fs_dir_close(dir);
    return 1;
}

/* =============================================================================
 * Path Submodule
 * ============================================================================= */

static int lfs_path_join(lua_State *L)
{
    int n = lua_gettop(L);
    char current[CAIS_PATH_MAX];
    char temp[CAIS_PATH_MAX];
    int i;

    if (n == 0) {
        lua_pushliteral(L, "");
        return 1;
    }

    current[0] = '\0';
    for (i = 1; i <= n; i++) {
        const char *part = luaL_checkstring(L, i);
        if (cais_fs_path_join(current, part, temp, sizeof(temp)) != CAIS_OK) {
            return luaL_error(L, "path too long during join");
        }
        strcpy(current, temp);
    }

    lua_pushstring(L, current);
    return 1;
}

static int lfs_path_basename(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char out[CAIS_PATH_MAX];
    if (cais_fs_path_basename(path, out, sizeof(out)) != CAIS_OK) {
        return luaL_error(L, "basename failed");
    }
    lua_pushstring(L, out);
    return 1;
}

static int lfs_path_dirname(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char out[CAIS_PATH_MAX];
    if (cais_fs_path_dirname(path, out, sizeof(out)) != CAIS_OK) {
        return luaL_error(L, "dirname failed");
    }
    lua_pushstring(L, out);
    return 1;
}

static int lfs_path_ext(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char out[CAIS_PATH_MAX];
    if (cais_fs_path_ext(path, out, sizeof(out)) != CAIS_OK) {
        return luaL_error(L, "ext failed");
    }
    lua_pushstring(L, out);
    return 1;
}

static int lfs_path_normalize(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    char out[CAIS_PATH_MAX];
    if (cais_fs_path_normalize(path, out, sizeof(out)) != CAIS_OK) {
        return luaL_error(L, "normalize failed");
    }
    lua_pushstring(L, out);
    return 1;
}

static int lfs_path_is_absolute(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, cais_fs_path_is_absolute(path) == CAIS_TRUE);
    return 1;
}

/* =============================================================================
 * File Streams (cais.file Userdata)
 * ============================================================================= */

static int lcais_file_read(lua_State *L)
{
    lcais_file_t *f = (lcais_file_t *)luaL_checkudata(L, 1, CAIS_FILE_MT);
    lua_Integer max_bytes = luaL_optinteger(L, 2, -1);
    char *buf;
    size_t count;
    size_t read_bytes = 0;
    cais_status_t st;

    if (f->handle == NULL) {
        return luaL_error(L, "attempt to read from closed file");
    }

    if (max_bytes == 0) {
        lua_pushliteral(L, "");
        return 1;
    }

    if (max_bytes < 0) {
        /* Read to end of file */
        long cur = 0, end = 0;
        cais_file_tell(f->handle, &cur);
        cais_file_seek(f->handle, 0, CAIS_FS_SEEK_END, &end);
        cais_file_seek(f->handle, cur, CAIS_FS_SEEK_SET, NULL);
        count = (end > cur) ? (size_t)(end - cur) : 4096;
    } else {
        count = (size_t)max_bytes;
    }

    buf = (char *)malloc(count);
    if (buf == NULL) {
        return luaL_error(L, "out of memory allocating read buffer");
    }

    st = cais_file_read(f->handle, buf, count, &read_bytes);
    if (st == CAIS_EOF || read_bytes == 0) {
        free(buf);
        lua_pushnil(L);
        return 1;
    }
    if (st != CAIS_OK) {
        free(buf);
        lua_pushnil(L);
        lua_pushliteral(L, "read error");
        return 2;
    }

    lua_pushlstring(L, buf, read_bytes);
    free(buf);
    return 1;
}

static int lcais_file_write(lua_State *L)
{
    lcais_file_t *f = (lcais_file_t *)luaL_checkudata(L, 1, CAIS_FILE_MT);
    size_t len = 0;
    const char *data = luaL_checklstring(L, 2, &len);
    size_t written = 0;

    if (f->handle == NULL) {
        return luaL_error(L, "attempt to write to closed file");
    }

    if (cais_file_write(f->handle, data, len, &written) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushliteral(L, "write error");
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)written);
    return 1;
}

static int lcais_file_seek(lua_State *L)
{
    lcais_file_t *f = (lcais_file_t *)luaL_checkudata(L, 1, CAIS_FILE_MT);
    const char *whence_str = luaL_optstring(L, 2, "cur");
    long offset = (long)luaL_optinteger(L, 3, 0);
    int whence = CAIS_FS_SEEK_CUR;
    long new_pos = 0;

    if (f->handle == NULL) {
        return luaL_error(L, "attempt to seek in closed file");
    }

    if (strcmp(whence_str, "set") == 0) whence = CAIS_FS_SEEK_SET;
    else if (strcmp(whence_str, "cur") == 0) whence = CAIS_FS_SEEK_CUR;
    else if (strcmp(whence_str, "end") == 0) whence = CAIS_FS_SEEK_END;
    else return luaL_error(L, "invalid seek origin '%s'", whence_str);

    if (cais_file_seek(f->handle, offset, whence, &new_pos) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushliteral(L, "seek error");
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)new_pos);
    return 1;
}

static int lcais_file_tell(lua_State *L)
{
    lcais_file_t *f = (lcais_file_t *)luaL_checkudata(L, 1, CAIS_FILE_MT);
    long pos = 0;

    if (f->handle == NULL) {
        return luaL_error(L, "attempt to tell on closed file");
    }

    if (cais_file_tell(f->handle, &pos) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushliteral(L, "tell error");
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)pos);
    return 1;
}

static int lcais_file_flush(lua_State *L)
{
    lcais_file_t *f = (lcais_file_t *)luaL_checkudata(L, 1, CAIS_FILE_MT);
    if (f->handle == NULL) {
        return luaL_error(L, "attempt to flush closed file");
    }
    if (cais_file_flush(f->handle) != CAIS_OK) {
        lua_pushnil(L);
        lua_pushliteral(L, "flush error");
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int lcais_file_close(lua_State *L)
{
    lcais_file_t *f = (lcais_file_t *)luaL_checkudata(L, 1, CAIS_FILE_MT);
    if (f->handle != NULL) {
        cais_file_close(f->handle);
        f->handle = NULL;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int file_gc(lua_State *L)
{
    lcais_file_t *f = (lcais_file_t *)luaL_testudata(L, 1, CAIS_FILE_MT);
    if (f != NULL && f->handle != NULL) {
        cais_file_close(f->handle);
        f->handle = NULL;
    }
    return 0;
}

static int lcais_file_tostring(lua_State *L)
{
    lcais_file_t *f = (lcais_file_t *)luaL_checkudata(L, 1, CAIS_FILE_MT);
    if (f->handle == NULL) {
        lua_pushliteral(L, "cais.filesystem.file (closed)");
    } else {
        lua_pushliteral(L, "cais.filesystem.file (open)");
    }
    return 1;
}

static int lfs_open(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    const char *mode = luaL_optstring(L, 2, "r");
    char err_buf[128];
    cais_file_t *file;
    lcais_file_t *ud;
    err_buf[0] = '\0';

    file = cais_file_open(path, mode, err_buf, sizeof(err_buf));
    if (file == NULL) {
        lua_pushnil(L);
        lua_pushstring(L, err_buf[0] ? err_buf : "failed to open file");
        return 2;
    }

    ud = (lcais_file_t *)lua_newuserdatauv(L, sizeof(lcais_file_t), 0);
    ud->handle = file;

    luaL_getmetatable(L, CAIS_FILE_MT);
    lua_setmetatable(L, -2);
    return 1;
}

/* =============================================================================
 * Module Registration
 * ============================================================================= */

static const luaL_Reg file_methods[] = {
    {"read",     lcais_file_read},
    {"write",    lcais_file_write},
    {"seek",     lcais_file_seek},
    {"tell",     lcais_file_tell},
    {"flush",    lcais_file_flush},
    {"close",    lcais_file_close},
    {"__gc",     file_gc},
    {"__close",  file_gc},
    {"__tostring", lcais_file_tostring},
    {NULL, NULL}
};

static const luaL_Reg dir_methods[] = {
    {"__gc",    dir_gc},
    {"__close", dir_gc},
    {NULL, NULL}
};

static const luaL_Reg fs_path_funcs[] = {
    {"join",        lfs_path_join},
    {"basename",    lfs_path_basename},
    {"dirname",     lfs_path_dirname},
    {"ext",         lfs_path_ext},
    {"extension",   lfs_path_ext},
    {"normalize",   lfs_path_normalize},
    {"is_absolute", lfs_path_is_absolute},
    {NULL, NULL}
};

static const luaL_Reg fs_funcs[] = {
    {"exists",      lfs_exists},
    {"stat",        lfs_stat},
    {"lstat",       lfs_lstat},
    {"is_file",     lfs_is_file},
    {"is_dir",      lfs_is_dir},
    {"is_symlink",  lfs_is_symlink},
    {"file_size",   lfs_file_size},
    {"read",        lfs_read},
    {"write",       lfs_write},
    {"append",      lfs_append},
    {"mkdir",       lfs_mkdir},
    {"rmdir",       lfs_rmdir},
    {"remove",      lfs_remove},
    {"rename",      lfs_rename},
    {"copy",        lfs_copy},
    {"chmod",       lfs_chmod},
    {"cwd",         lfs_cwd},
    {"chdir",       lfs_chdir},
    {"temp_dir",    lfs_temp_dir},
    {"dir",         lfs_dir},
    {"list",        lfs_list},
    {"open",        lfs_open},
    {NULL, NULL}
};

int luaopen_cais_filesystem(lua_State *L);
int luaopen_cais_fs(lua_State *L);

int luaopen_cais_filesystem(lua_State *L)
{
    /* 1. Register cais.filesystem.file metatable */
    if (luaL_newmetatable(L, CAIS_FILE_MT)) {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
#if LUA_VERSION_NUM >= 502
        luaL_setfuncs(L, file_methods, 0);
#else
        luaL_register(L, NULL, file_methods);
#endif
    }
    lua_pop(L, 1);

    /* 2. Register cais.filesystem.dir metatable */
    if (luaL_newmetatable(L, CAIS_DIR_MT)) {
#if LUA_VERSION_NUM >= 502
        luaL_setfuncs(L, dir_methods, 0);
#else
        luaL_register(L, NULL, dir_methods);
#endif
    }
    lua_pop(L, 1);

    /* 3. Create module table */
#if LUA_VERSION_NUM >= 502
    luaL_newlib(L, fs_funcs);
#else
    lua_newtable(L);
    luaL_register(L, NULL, fs_funcs);
#endif

    /* 4. Subtable fs.path */
#if LUA_VERSION_NUM >= 502
    luaL_newlib(L, fs_path_funcs);
#else
    lua_newtable(L);
    luaL_register(L, NULL, fs_path_funcs);
#endif
    {
        char sep_str[2];
        sep_str[0] = cais_fs_path_separator();
        sep_str[1] = '\0';
        lua_pushstring(L, sep_str);
        lua_setfield(L, -2, "separator");
    }
    lua_setfield(L, -2, "path");

    /* 5. Subtable fs.type */
    lua_createtable(L, 0, 7);
    lua_pushinteger(L, CAIS_FS_TYPE_UNKNOWN); lua_setfield(L, -2, "UNKNOWN");
    lua_pushinteger(L, CAIS_FS_TYPE_FILE);    lua_setfield(L, -2, "FILE");
    lua_pushinteger(L, CAIS_FS_TYPE_DIR);     lua_setfield(L, -2, "DIR");
    lua_pushinteger(L, CAIS_FS_TYPE_SYMLINK); lua_setfield(L, -2, "SYMLINK");
    lua_pushinteger(L, CAIS_FS_TYPE_DEVICE);  lua_setfield(L, -2, "DEVICE");
    lua_pushinteger(L, CAIS_FS_TYPE_FIFO);    lua_setfield(L, -2, "FIFO");
    lua_pushinteger(L, CAIS_FS_TYPE_SOCKET);  lua_setfield(L, -2, "SOCKET");
    lua_setfield(L, -2, "type");

    /* 6. Subtable fs.status */
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, CAIS_FS_OK);          lua_setfield(L, -2, "OK");
    lua_pushinteger(L, CAIS_FS_ERROR);       lua_setfield(L, -2, "ERROR");
    lua_pushinteger(L, CAIS_FS_UNSUPPORTED); lua_setfield(L, -2, "UNSUPPORTED");
    lua_pushinteger(L, CAIS_FS_EOF);         lua_setfield(L, -2, "EOF");
    lua_setfield(L, -2, "status");

    return 1;
}

int luaopen_cais_fs(lua_State *L)
{
    return luaopen_cais_filesystem(L);
}
