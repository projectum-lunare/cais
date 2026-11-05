/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cais/platform.h>

#if CAIS_IS_AMIGA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dos/dos.h>
#include <exec/types.h>
#include <proto/dos.h>

#include "fs_internal.h"

struct cais_fs_dir {
    BPTR lock;
    struct FileInfoBlock fib;
    int first;
    int finished;
    char path[CAIS_PATH_MAX];
};

cais_status_t cais_pal_fs_stat(const char *path, cais_fs_stat_t *st, cais_bool_t follow_links, char *err_buf, size_t err_cap)
{
    BPTR lock;
    struct FileInfoBlock fib;
    (void)follow_links;

    if (path == NULL || st == NULL) {
        cais_set_err(err_buf, err_cap, "invalid null argument to stat");
        return CAIS_ERROR;
    }

    memset(st, 0, sizeof(*st));
    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (lock == 0) {
        cais_set_err(err_buf, err_cap, "Lock failed (file or directory not found)");
        return CAIS_ERROR;
    }

    if (!Examine(lock, &fib)) {
        UnLock(lock);
        cais_set_err(err_buf, err_cap, "Examine failed on Amiga lock");
        return CAIS_ERROR;
    }

    UnLock(lock);

    st->size = (cais_fsize_t)fib.fib_Size;
    st->mtime = 0;
    st->atime = 0;
    st->ctime = 0;
    st->mode = (int)fib.fib_Protection;
    st->is_readonly = (fib.fib_Protection & FIBF_WRITE) ? CAIS_TRUE : CAIS_FALSE;

    if (fib.fib_DirEntryType > 0) {
        st->type = CAIS_FS_TYPE_DIR;
    } else {
        st->type = CAIS_FS_TYPE_FILE;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_mkdir(const char *path, int mode, char *err_buf, size_t err_cap)
{
    BPTR lock;
    (void)mode;
    lock = CreateDir((CONST_STRPTR)path);
    if (lock == 0) {
        cais_set_err(err_buf, err_cap, "CreateDir failed on AmigaDOS");
        return CAIS_ERROR;
    }
    UnLock(lock);
    return CAIS_OK;
}

cais_status_t cais_pal_fs_rmdir(const char *path, char *err_buf, size_t err_cap)
{
    if (!DeleteFile((CONST_STRPTR)path)) {
        cais_set_err(err_buf, err_cap, "DeleteFile failed on AmigaDOS");
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_unlink(const char *path, char *err_buf, size_t err_cap)
{
    if (!DeleteFile((CONST_STRPTR)path)) {
        cais_set_err(err_buf, err_cap, "DeleteFile failed on AmigaDOS");
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_rename(const char *oldpath, const char *newpath, char *err_buf, size_t err_cap)
{
    if (!Rename((CONST_STRPTR)oldpath, (CONST_STRPTR)newpath)) {
        cais_set_err(err_buf, err_cap, "Rename failed on AmigaDOS");
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_chmod(const char *path, int mode, char *err_buf, size_t err_cap)
{
    if (!SetProtection((CONST_STRPTR)path, (LONG)mode)) {
        cais_set_err(err_buf, err_cap, "SetProtection failed on AmigaDOS");
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_fs_dir_t *cais_pal_fs_dir_open(const char *path, char *err_buf, size_t err_cap)
{
    cais_fs_dir_t *dir;
    BPTR lock;

    if (path == NULL) {
        cais_set_err(err_buf, err_cap, "path is null");
        return NULL;
    }

    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (lock == 0) {
        cais_set_err(err_buf, err_cap, "Lock failed on directory");
        return NULL;
    }

    dir = (cais_fs_dir_t *)malloc(sizeof(cais_fs_dir_t));
    if (dir == NULL) {
        UnLock(lock);
        cais_set_err(err_buf, err_cap, "out of memory allocating directory handle");
        return NULL;
    }

    memset(dir, 0, sizeof(*dir));
    dir->lock = lock;
    strncpy(dir->path, path, sizeof(dir->path) - 1);

    if (!Examine(lock, &dir->fib)) {
        UnLock(lock);
        free(dir);
        cais_set_err(err_buf, err_cap, "Examine failed on directory");
        return NULL;
    }

    dir->first = 1;
    dir->finished = 0;
    return dir;
}

cais_status_t cais_pal_fs_dir_next(cais_fs_dir_t *dir, cais_fs_dirent_t *ent)
{
    if (dir == NULL || ent == NULL || dir->finished) {
        return CAIS_EOF;
    }

    if (!ExNext(dir->lock, &dir->fib)) {
        dir->finished = 1;
        return CAIS_EOF;
    }

    strncpy(ent->name, (const char *)dir->fib.fib_FileName, sizeof(ent->name) - 1);
    ent->name[sizeof(ent->name) - 1] = '\0';

    if (dir->fib.fib_DirEntryType > 0) {
        ent->type = CAIS_FS_TYPE_DIR;
    } else {
        ent->type = CAIS_FS_TYPE_FILE;
    }
    return CAIS_OK;
}

void cais_pal_fs_dir_close(cais_fs_dir_t *dir)
{
    if (dir != NULL) {
        if (dir->lock != 0) {
            UnLock(dir->lock);
        }
        free(dir);
    }
}

cais_status_t cais_pal_fs_getcwd(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    BPTR lock = Lock((CONST_STRPTR)"", SHARED_LOCK);
    if (lock == 0) {
        cais_set_err(err_buf, err_cap, "Lock on current dir failed");
        return CAIS_ERROR;
    }
    if (!NameFromLock(lock, (STRPTR)buf, (LONG)cap)) {
        UnLock(lock);
        cais_set_err(err_buf, err_cap, "NameFromLock failed");
        return CAIS_ERROR;
    }
    UnLock(lock);
    return CAIS_OK;
}

cais_status_t cais_pal_fs_setcwd(const char *path, char *err_buf, size_t err_cap)
{
    BPTR lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    BPTR oldlock;
    if (lock == 0) {
        cais_set_err(err_buf, err_cap, "Lock on path failed");
        return CAIS_ERROR;
    }
    oldlock = CurrentDir(lock);
    UnLock(oldlock);
    return CAIS_OK;
}

cais_status_t cais_pal_fs_temp_dir(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    if (cap < 3) {
        cais_set_err(err_buf, err_cap, "buffer too small");
        return CAIS_ERROR;
    }
    strcpy(buf, "T:");
    return CAIS_OK;
}

#endif /* CAIS_IS_AMIGA */
