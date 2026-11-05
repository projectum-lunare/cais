/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cais/platform.h>

#if CAIS_IS_KOLIBRI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs_internal.h"

struct cais_fs_dir {
    char path[CAIS_PATH_MAX];
    int finished;
};

cais_status_t cais_pal_fs_stat(const char *path, cais_fs_stat_t *st, cais_bool_t follow_links, char *err_buf, size_t err_cap)
{
    (void)follow_links;
    if (path == NULL || st == NULL) {
        cais_set_err(err_buf, err_cap, "invalid argument");
        return CAIS_ERROR;
    }
    memset(st, 0, sizeof(*st));
    st->type = CAIS_FS_TYPE_FILE;
    st->mode = 0666;
    st->is_readonly = CAIS_FALSE;
    return CAIS_OK;
}

cais_status_t cais_pal_fs_mkdir(const char *path, int mode, char *err_buf, size_t err_cap)
{
    (void)path; (void)mode;
    cais_set_err(err_buf, err_cap, "mkdir not implemented on KolibriOS");
    return CAIS_UNSUPPORTED;
}

cais_status_t cais_pal_fs_rmdir(const char *path, char *err_buf, size_t err_cap)
{
    (void)path;
    cais_set_err(err_buf, err_cap, "rmdir not implemented on KolibriOS");
    return CAIS_UNSUPPORTED;
}

cais_status_t cais_pal_fs_unlink(const char *path, char *err_buf, size_t err_cap)
{
    (void)path;
    cais_set_err(err_buf, err_cap, "unlink not implemented on KolibriOS");
    return CAIS_UNSUPPORTED;
}

cais_status_t cais_pal_fs_rename(const char *oldpath, const char *newpath, char *err_buf, size_t err_cap)
{
    (void)oldpath; (void)newpath;
    cais_set_err(err_buf, err_cap, "rename not implemented on KolibriOS");
    return CAIS_UNSUPPORTED;
}

cais_status_t cais_pal_fs_chmod(const char *path, int mode, char *err_buf, size_t err_cap)
{
    (void)path; (void)mode;
    cais_set_err(err_buf, err_cap, "chmod not implemented on KolibriOS");
    return CAIS_UNSUPPORTED;
}

cais_fs_dir_t *cais_pal_fs_dir_open(const char *path, char *err_buf, size_t err_cap)
{
    (void)path;
    cais_set_err(err_buf, err_cap, "dir_open not implemented on KolibriOS");
    return NULL;
}

cais_status_t cais_pal_fs_dir_next(cais_fs_dir_t *dir, cais_fs_dirent_t *ent)
{
    (void)dir; (void)ent;
    return CAIS_EOF;
}

void cais_pal_fs_dir_close(cais_fs_dir_t *dir)
{
    if (dir) free(dir);
}

cais_status_t cais_pal_fs_getcwd(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    (void)err_buf; (void)err_cap;
    if (cap < 2) return CAIS_ERROR;
    strcpy(buf, "/");
    return CAIS_OK;
}

cais_status_t cais_pal_fs_setcwd(const char *path, char *err_buf, size_t err_cap)
{
    (void)path; (void)err_buf; (void)err_cap;
    return CAIS_OK;
}

cais_status_t cais_pal_fs_temp_dir(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    (void)err_buf; (void)err_cap;
    if (cap < 6) return CAIS_ERROR;
    strcpy(buf, "/tmp");
    return CAIS_OK;
}

#endif /* CAIS_IS_KOLIBRI */
