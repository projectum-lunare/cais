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

#if CAIS_IS_OS2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>

#include "fs_internal.h"

struct cais_fs_dir {
    char path[CAIS_PATH_MAX];
    int finished;
};

cais_status_t cais_pal_fs_stat(const char *path, cais_fs_stat_t *st, cais_bool_t follow_links, char *err_buf, size_t err_cap)
{
    struct stat s;
    (void)follow_links;

    if (path == NULL || st == NULL) {
        cais_set_err(err_buf, err_cap, "invalid argument");
        return CAIS_ERROR;
    }

    memset(st, 0, sizeof(*st));
    if (stat(path, &s) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "stat failed: %s", strerror(errno));
        return CAIS_ERROR;
    }

    st->size = (cais_fsize_t)s.st_size;
    st->mtime = (long)s.st_mtime;
    st->atime = (long)s.st_atime;
    st->ctime = (long)s.st_ctime;
    st->mode = (int)(s.st_mode & 07777);
    st->is_readonly = !(s.st_mode & 0222) ? CAIS_TRUE : CAIS_FALSE;

    if (s.st_mode & S_IFDIR) {
        st->type = CAIS_FS_TYPE_DIR;
    } else {
        st->type = CAIS_FS_TYPE_FILE;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_mkdir(const char *path, int mode, char *err_buf, size_t err_cap)
{
    (void)mode;
    if (mkdir(path) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "mkdir failed: %s", strerror(errno));
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_rmdir(const char *path, char *err_buf, size_t err_cap)
{
    if (rmdir(path) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "rmdir failed: %s", strerror(errno));
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_unlink(const char *path, char *err_buf, size_t err_cap)
{
    if (unlink(path) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "unlink failed: %s", strerror(errno));
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_rename(const char *oldpath, const char *newpath, char *err_buf, size_t err_cap)
{
    if (rename(oldpath, newpath) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "rename failed: %s", strerror(errno));
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_chmod(const char *path, int mode, char *err_buf, size_t err_cap)
{
    if (chmod(path, mode) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "chmod failed: %s", strerror(errno));
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_fs_dir_t *cais_pal_fs_dir_open(const char *path, char *err_buf, size_t err_cap)
{
    (void)path;
    cais_set_err(err_buf, err_cap, "dir_open not implemented on OS/2");
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
    if (getcwd(buf, (int)cap) == NULL) {
        cais_set_err_fmt(err_buf, err_cap, "getcwd failed: %s", strerror(errno));
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_setcwd(const char *path, char *err_buf, size_t err_cap)
{
    if (chdir(path) != 0) {
        cais_set_err_fmt(err_buf, err_cap, "chdir failed: %s", strerror(errno));
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_temp_dir(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    const char *tmp = getenv("TEMP");
    if (tmp == NULL || tmp[0] == '\0') tmp = getenv("TMP");
    if (tmp == NULL || tmp[0] == '\0') tmp = "C:\\TEMP";

    if (strlen(tmp) + 1 > cap) return CAIS_ERROR;
    strcpy(buf, tmp);
    return CAIS_OK;
}

#endif /* CAIS_IS_OS2 */
