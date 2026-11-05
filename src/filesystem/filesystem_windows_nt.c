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

#if CAIS_IS_WINDOWS && !CAIS_IS_WINDOWS_CE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>

#include "fs_internal.h"

struct cais_fs_dir {
    HANDLE hFind;
    WIN32_FIND_DATAA findData;
    int first;
    int finished;
    char path[CAIS_PATH_MAX];
};

static void win_filetime_to_epoch(const FILETIME *ft, long *out_epoch)
{
    ULARGE_INTEGER ull;
    ull.LowPart = ft->dwLowDateTime;
    ull.HighPart = ft->dwHighDateTime;
    if (ull.QuadPart >= 116444736000000000ULL) {
        *out_epoch = (long)((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
    } else {
        *out_epoch = 0;
    }
}

cais_status_t cais_pal_fs_stat(const char *path, cais_fs_stat_t *st, cais_bool_t follow_links, char *err_buf, size_t err_cap)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;
    (void)follow_links;

    if (path == NULL || st == NULL) {
        cais_set_err(err_buf, err_cap, "invalid null argument to stat");
        return CAIS_ERROR;
    }

    memset(st, 0, sizeof(*st));

    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fad)) {
        cais_set_err_fmt(err_buf, err_cap, "GetFileAttributesEx failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }

    st->size = (((cais_fsize_t)fad.nFileSizeHigh) << 32) | (cais_fsize_t)fad.nFileSizeLow;
    win_filetime_to_epoch(&fad.ftLastWriteTime, &st->mtime);
    win_filetime_to_epoch(&fad.ftLastAccessTime, &st->atime);
    win_filetime_to_epoch(&fad.ftCreationTime, &st->ctime);

    st->is_readonly = (fad.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? CAIS_TRUE : CAIS_FALSE;

    if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        st->type = CAIS_FS_TYPE_DIR;
        st->mode = st->is_readonly ? 0555 : 0777;
    } else {
        st->type = CAIS_FS_TYPE_FILE;
        st->mode = st->is_readonly ? 0444 : 0666;
    }

#if defined(FILE_ATTRIBUTE_REPARSE_POINT)
    if (fad.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        st->type = CAIS_FS_TYPE_SYMLINK;
    }
#endif

    return CAIS_OK;
}

cais_status_t cais_pal_fs_mkdir(const char *path, int mode, char *err_buf, size_t err_cap)
{
    (void)mode;
    if (!CreateDirectoryA(path, NULL)) {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) {
            cais_set_err(err_buf, err_cap, "directory already exists");
        } else {
            cais_set_err_fmt(err_buf, err_cap, "CreateDirectory failed (code %lu)", (unsigned long)err);
        }
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_rmdir(const char *path, char *err_buf, size_t err_cap)
{
    if (!RemoveDirectoryA(path)) {
        cais_set_err_fmt(err_buf, err_cap, "RemoveDirectory failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_unlink(const char *path, char *err_buf, size_t err_cap)
{
    if (!DeleteFileA(path)) {
        cais_set_err_fmt(err_buf, err_cap, "DeleteFile failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_rename(const char *oldpath, const char *newpath, char *err_buf, size_t err_cap)
{
    if (!MoveFileExA(oldpath, newpath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        cais_set_err_fmt(err_buf, err_cap, "MoveFile failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_chmod(const char *path, int mode, char *err_buf, size_t err_cap)
{
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        cais_set_err_fmt(err_buf, err_cap, "GetFileAttributes failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }

    if (mode & 0222) {
        attr &= ~FILE_ATTRIBUTE_READONLY;
    } else {
        attr |= FILE_ATTRIBUTE_READONLY;
    }

    if (!SetFileAttributesA(path, attr)) {
        cais_set_err_fmt(err_buf, err_cap, "SetFileAttributes failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_fs_dir_t *cais_pal_fs_dir_open(const char *path, char *err_buf, size_t err_cap)
{
    char search_pattern[CAIS_PATH_MAX];
    cais_fs_dir_t *dir;
    size_t plen;

    if (path == NULL) {
        cais_set_err(err_buf, err_cap, "path is null");
        return NULL;
    }

    plen = strlen(path);
    if (plen + 3 >= sizeof(search_pattern)) {
        cais_set_err(err_buf, err_cap, "path too long for search pattern");
        return NULL;
    }

    strcpy(search_pattern, path);
    if (plen > 0 && search_pattern[plen - 1] != '/' && search_pattern[plen - 1] != '\\') {
        strcat(search_pattern, "\\*");
    } else {
        strcat(search_pattern, "*");
    }

    dir = (cais_fs_dir_t *)malloc(sizeof(cais_fs_dir_t));
    if (dir == NULL) {
        cais_set_err(err_buf, err_cap, "out of memory allocating directory handle");
        return NULL;
    }

    memset(dir, 0, sizeof(*dir));
    strncpy(dir->path, path, sizeof(dir->path) - 1);

    dir->hFind = FindFirstFileA(search_pattern, &dir->findData);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        free(dir);
        cais_set_err_fmt(err_buf, err_cap, "FindFirstFile failed (code %lu)", (unsigned long)err);
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
            if (!FindNextFileA(dir->hFind, &dir->findData)) {
                dir->finished = 1;
                return CAIS_EOF;
            }
        }
        dir->first = 0;

        if (strcmp(dir->findData.cFileName, ".") == 0 || strcmp(dir->findData.cFileName, "..") == 0) {
            continue;
        }

        strncpy(ent->name, dir->findData.cFileName, sizeof(ent->name) - 1);
        ent->name[sizeof(ent->name) - 1] = '\0';

        if (dir->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
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
        if (dir->hFind != INVALID_HANDLE_VALUE && dir->hFind != NULL) {
            FindClose(dir->hFind);
        }
        free(dir);
    }
}

cais_status_t cais_pal_fs_getcwd(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    DWORD len = GetCurrentDirectoryA((DWORD)cap, buf);
    if (len == 0 || len >= cap) {
        cais_set_err_fmt(err_buf, err_cap, "GetCurrentDirectory failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_setcwd(const char *path, char *err_buf, size_t err_cap)
{
    if (!SetCurrentDirectoryA(path)) {
        cais_set_err_fmt(err_buf, err_cap, "SetCurrentDirectory failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_pal_fs_temp_dir(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    DWORD len = GetTempPathA((DWORD)cap, buf);
    if (len == 0 || len >= cap) {
        cais_set_err_fmt(err_buf, err_cap, "GetTempPath failed (code %lu)", (unsigned long)GetLastError());
        return CAIS_ERROR;
    }
    if (len > 1 && (buf[len - 1] == '\\' || buf[len - 1] == '/')) {
        buf[len - 1] = '\0';
    }
    return CAIS_OK;
}

#endif /* CAIS_IS_WINDOWS && !CAIS_IS_WINDOWS_CE */
