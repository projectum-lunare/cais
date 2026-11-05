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

#if CAIS_IS_DOS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <dos.h>

#include "fs_internal.h"

struct cais_fs_dir {
    char path[CAIS_PATH_MAX];
    struct find_t c_file;
    int first;
    int finished;
};

cais_status_t cais_pal_fs_stat(const char *path, cais_fs_stat_t *st, cais_bool_t follow_links, char *err_buf, size_t err_cap)
{
    struct stat s;
    (void)follow_links;

    if (path == NULL || st == NULL) {
        cais_set_err(err_buf, err_cap, "invalid null argument to stat");
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
    char pattern[CAIS_PATH_MAX];
    cais_fs_dir_t *dir;
    size_t plen;

    if (path == NULL) {
        cais_set_err(err_buf, err_cap, "path is null");
        return NULL;
    }

    plen = strlen(path);
    if (plen + 5 >= sizeof(pattern)) {
        cais_set_err(err_buf, err_cap, "path too long");
        return NULL;
    }

    strcpy(pattern, path);
    if (plen > 0 && pattern[plen - 1] != '/' && pattern[plen - 1] != '\\') {
        strcat(pattern, "\\*.*");
    } else {
        strcat(pattern, "*.*");
    }

    dir = (cais_fs_dir_t *)malloc(sizeof(cais_fs_dir_t));
    if (dir == NULL) {
        cais_set_err(err_buf, err_cap, "out of memory allocating directory handle");
        return NULL;
    }

    memset(dir, 0, sizeof(*dir));
    strncpy(dir->path, path, sizeof(dir->path) - 1);

    if (_dos_findfirst(pattern, _A_NORMAL | _A_SUBDIR | _A_RDONLY, &dir->c_file) != 0) {
        free(dir);
        cais_set_err(err_buf, err_cap, "no files found or invalid directory");
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

    while (1) {
        if (!dir->first) {
            if (_dos_findnext(&dir->c_file) != 0) {
                dir->finished = 1;
                return CAIS_EOF;
            }
        }
        dir->first = 0;

        if (strcmp(dir->c_file.name, ".") == 0 || strcmp(dir->c_file.name, "..") == 0) {
            continue;
        }

        strncpy(ent->name, dir->c_file.name, sizeof(ent->name) - 1);
        ent->name[sizeof(ent->name) - 1] = '\0';

        if (dir->c_file.attrib & _A_SUBDIR) {
            ent->type = CAIS_FS_TYPE_DIR;
        } else {
            ent->type = CAIS_FS_TYPE_FILE;
        }

        return CAIS_OK;
    }
}

void cais_pal_fs_dir_close(cais_fs_dir_t *dir)
{
    if (dir != NULL) {
        free(dir);
    }
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

    if (strlen(tmp) + 1 > cap) {
        cais_set_err(err_buf, err_cap, "buffer too small for temp dir path");
        return CAIS_ERROR;
    }
    strcpy(buf, tmp);
    return CAIS_OK;
}

#endif /* CAIS_IS_DOS */
