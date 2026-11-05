/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <cais/filesystem.h>
#include <cais/platform.h>
#include "fs_internal.h"

/* =============================================================================
 * 1. Path Algebra & Pure Normalization
 * ============================================================================= */

char cais_fs_path_separator(void)
{
#if CAIS_IS_WINDOWS || CAIS_IS_DOS || CAIS_IS_OS2
    return '\\';
#else
    return '/';
#endif
}

static int is_sep(char c)
{
    return (c == '/' || c == '\\');
}

cais_bool_t cais_fs_path_is_absolute(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return CAIS_FALSE;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return CAIS_TRUE;
    }
#if CAIS_IS_WINDOWS || CAIS_IS_DOS || CAIS_IS_OS2
    if (isalpha((unsigned char)path[0]) && path[1] == ':') {
        if (path[2] == '/' || path[2] == '\\' || path[2] == '\0') {
            return CAIS_TRUE;
        }
    }
#elif CAIS_IS_AMIGA
    {
        const char *p = path;
        while (*p) {
            if (*p == ':') return CAIS_TRUE;
            if (*p == '/') break;
            p++;
        }
    }
#endif
    return CAIS_FALSE;
}

cais_status_t cais_fs_path_join(const char *p1, const char *p2, char *out, size_t out_cap)
{
    size_t l1, l2;
    char sep;

    if (out == NULL || out_cap == 0) {
        return CAIS_ERROR;
    }

    if (p1 == NULL || p1[0] == '\0') {
        if (p2 == NULL || p2[0] == '\0') {
            out[0] = '\0';
            return CAIS_OK;
        }
        if (strlen(p2) >= out_cap) return CAIS_ERROR;
        strcpy(out, p2);
        return CAIS_OK;
    }

    if (p2 == NULL || p2[0] == '\0') {
        if (strlen(p1) >= out_cap) return CAIS_ERROR;
        strcpy(out, p1);
        return CAIS_OK;
    }

    if (cais_fs_path_is_absolute(p2)) {
        if (strlen(p2) >= out_cap) return CAIS_ERROR;
        strcpy(out, p2);
        return CAIS_OK;
    }

    l1 = strlen(p1);
    l2 = strlen(p2);
    sep = cais_fs_path_separator();

    if (is_sep(p1[l1 - 1])) {
        if (l1 + l2 >= out_cap) return CAIS_ERROR;
        memcpy(out, p1, l1);
        memcpy(out + l1, p2, l2 + 1);
    } else {
        if (l1 + 1 + l2 >= out_cap) return CAIS_ERROR;
        memcpy(out, p1, l1);
        out[l1] = sep;
        memcpy(out + l1 + 1, p2, l2 + 1);
    }

    return CAIS_OK;
}

cais_status_t cais_fs_path_basename(const char *path, char *out, size_t out_cap)
{
    size_t len;
    long i, end;

    if (path == NULL || out == NULL || out_cap == 0) {
        return CAIS_ERROR;
    }

    len = strlen(path);
    if (len == 0) {
        if (out_cap < 2) return CAIS_ERROR;
        strcpy(out, ".");
        return CAIS_OK;
    }

    /* Strip trailing slashes */
    end = (long)len - 1;
    while (end > 0 && is_sep(path[end])) {
        end--;
    }

    if (end == 0 && is_sep(path[0])) {
        if (out_cap < 2) return CAIS_ERROR;
        out[0] = path[0];
        out[1] = '\0';
        return CAIS_OK;
    }

    i = end;
    while (i >= 0 && !is_sep(path[i])) {
        i--;
    }

    {
        size_t blen = (size_t)(end - i);
        if (blen >= out_cap) return CAIS_ERROR;
        memcpy(out, path + i + 1, blen);
        out[blen] = '\0';
    }

    return CAIS_OK;
}

cais_status_t cais_fs_path_dirname(const char *path, char *out, size_t out_cap)
{
    size_t len;
    long end, i;

    if (path == NULL || out == NULL || out_cap == 0) {
        return CAIS_ERROR;
    }

    len = strlen(path);
    if (len == 0) {
        if (out_cap < 2) return CAIS_ERROR;
        strcpy(out, ".");
        return CAIS_OK;
    }

    end = (long)len - 1;
    while (end > 0 && is_sep(path[end])) {
        end--;
    }

    i = end;
    while (i >= 0 && !is_sep(path[i])) {
        i--;
    }

    if (i < 0) {
        if (out_cap < 2) return CAIS_ERROR;
        strcpy(out, ".");
        return CAIS_OK;
    }

    /* Collapse multiple separators */
    while (i > 0 && is_sep(path[i - 1])) {
        i--;
    }

    if (i == 0 && is_sep(path[0])) {
        if (out_cap < 2) return CAIS_ERROR;
        out[0] = path[0];
        out[1] = '\0';
        return CAIS_OK;
    }

    if ((size_t)i >= out_cap) return CAIS_ERROR;
    memcpy(out, path, (size_t)i);
    out[i] = '\0';
    return CAIS_OK;
}

cais_status_t cais_fs_path_ext(const char *path, char *out, size_t out_cap)
{
    char base[CAIS_PATH_MAX];
    long i, len;

    if (path == NULL || out == NULL || out_cap == 0) {
        return CAIS_ERROR;
    }

    out[0] = '\0';
    if (cais_fs_path_basename(path, base, sizeof(base)) != CAIS_OK) {
        return CAIS_ERROR;
    }

    len = (long)strlen(base);
    if (len <= 1) {
        return CAIS_OK;
    }

    for (i = len - 1; i > 0; i--) {
        if (base[i] == '.') {
            size_t ext_len = (size_t)(len - i);
            if (ext_len >= out_cap) return CAIS_ERROR;
            memcpy(out, base + i, ext_len + 1);
            return CAIS_OK;
        }
    }

    return CAIS_OK;
}

cais_status_t cais_fs_path_normalize(const char *path, char *out, size_t out_cap)
{
    char sep = cais_fs_path_separator();
    char buf[CAIS_PATH_MAX];
    char *tokens[128];
    int token_count = 0;
    int is_abs;
    size_t i, len;
    char *p, *start;
    char prefix[8];
    size_t prefix_len = 0;

    if (path == NULL || out == NULL || out_cap == 0) {
        return CAIS_ERROR;
    }

    len = strlen(path);
    if (len >= sizeof(buf)) {
        return CAIS_ERROR;
    }

    strcpy(buf, path);
    prefix[0] = '\0';

    /* Detect drive letter e.g. "C:\" */
    if (isalpha((unsigned char)buf[0]) && buf[1] == ':') {
        prefix[0] = buf[0];
        prefix[1] = ':';
        prefix[2] = '\0';
        prefix_len = 2;
        p = buf + 2;
    } else {
        p = buf;
    }

    is_abs = (*p == '/' || *p == '\\');

    /* Tokenize by / and \ */
    start = p;
    while (*start != '\0') {
        while (*start != '\0' && is_sep(*start)) {
            start++;
        }
        if (*start == '\0') break;

        p = start;
        while (*p != '\0' && !is_sep(*p)) {
            p++;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }

        if (strcmp(start, ".") == 0) {
            /* Skip current dir */
        } else if (strcmp(start, "..") == 0) {
            if (token_count > 0 && strcmp(tokens[token_count - 1], "..") != 0) {
                token_count--;
            } else if (!is_abs) {
                if (token_count < 128) {
                    tokens[token_count++] = start;
                }
            }
        } else {
            if (token_count < 128) {
                tokens[token_count++] = start;
            }
        }
        start = p;
    }

    /* Reconstruct path */
    out[0] = '\0';
    if (prefix_len > 0) {
        if (prefix_len >= out_cap) return CAIS_ERROR;
        strcpy(out, prefix);
    }

    if (is_abs) {
        size_t clen = strlen(out);
        if (clen + 1 >= out_cap) return CAIS_ERROR;
        out[clen] = sep;
        out[clen + 1] = '\0';
    }

    if (token_count == 0 && !is_abs) {
        if (strlen(out) + 2 >= out_cap) return CAIS_ERROR;
        strcat(out, ".");
        return CAIS_OK;
    }

    for (i = 0; i < (size_t)token_count; i++) {
        size_t cur_len = strlen(out);
        size_t t_len = strlen(tokens[i]);

        if (cur_len > 0 && out[cur_len - 1] != sep && out[cur_len - 1] != ':') {
            if (cur_len + 1 + t_len >= out_cap) return CAIS_ERROR;
            out[cur_len] = sep;
            out[cur_len + 1] = '\0';
        } else {
            if (cur_len + t_len >= out_cap) return CAIS_ERROR;
        }
        strcat(out, tokens[i]);
    }

    return CAIS_OK;
}

/* =============================================================================
 * 2. File & Directory Inspection (Metadata & Stat)
 * ============================================================================= */

cais_bool_t cais_fs_exists(const char *path)
{
    cais_fs_stat_t st;
    if (path == NULL || path[0] == '\0') {
        return CAIS_FALSE;
    }
    return (cais_pal_fs_stat(path, &st, CAIS_TRUE, NULL, 0) == CAIS_OK) ? CAIS_TRUE : CAIS_FALSE;
}

cais_status_t cais_fs_stat(const char *path, cais_fs_stat_t *st, char *err_buf, size_t err_cap)
{
    return cais_pal_fs_stat(path, st, CAIS_TRUE, err_buf, err_cap);
}

cais_status_t cais_fs_lstat(const char *path, cais_fs_stat_t *st, char *err_buf, size_t err_cap)
{
    return cais_pal_fs_stat(path, st, CAIS_FALSE, err_buf, err_cap);
}

cais_bool_t cais_fs_is_file(const char *path)
{
    cais_fs_stat_t st;
    if (cais_pal_fs_stat(path, &st, CAIS_TRUE, NULL, 0) == CAIS_OK) {
        return (st.type == CAIS_FS_TYPE_FILE) ? CAIS_TRUE : CAIS_FALSE;
    }
    return CAIS_FALSE;
}

cais_bool_t cais_fs_is_dir(const char *path)
{
    cais_fs_stat_t st;
    if (cais_pal_fs_stat(path, &st, CAIS_TRUE, NULL, 0) == CAIS_OK) {
        return (st.type == CAIS_FS_TYPE_DIR) ? CAIS_TRUE : CAIS_FALSE;
    }
    return CAIS_FALSE;
}

cais_bool_t cais_fs_is_symlink(const char *path)
{
    cais_fs_stat_t st;
    if (cais_pal_fs_stat(path, &st, CAIS_FALSE, NULL, 0) == CAIS_OK) {
        return (st.type == CAIS_FS_TYPE_SYMLINK) ? CAIS_TRUE : CAIS_FALSE;
    }
    return CAIS_FALSE;
}

cais_status_t cais_fs_file_size(const char *path, cais_fsize_t *out_size)
{
    cais_fs_stat_t st;
    if (out_size == NULL) return CAIS_ERROR;
    if (cais_pal_fs_stat(path, &st, 1, NULL, 0) == CAIS_OK) {
        *out_size = st.size;
        return CAIS_OK;
    }
    return CAIS_ERROR;
}

/* =============================================================================
 * 3. File System Operations & Mutation
 * ============================================================================= */

cais_status_t cais_fs_mkdir(const char *path, int mode, char *err_buf, size_t err_cap)
{
    if (path == NULL || path[0] == '\0') {
        cais_set_err(err_buf, err_cap, "path is empty");
        return CAIS_ERROR;
    }
    return cais_pal_fs_mkdir(path, mode, err_buf, err_cap);
}

cais_status_t cais_fs_mkdir_p(const char *path, int mode, char *err_buf, size_t err_cap)
{
    char tmp[CAIS_PATH_MAX];
    char *p;
    size_t len;

    if (path == NULL || path[0] == '\0') {
        cais_set_err(err_buf, err_cap, "path is empty");
        return CAIS_ERROR;
    }

    len = strlen(path);
    if (len >= sizeof(tmp)) {
        cais_set_err(err_buf, err_cap, "path too long");
        return CAIS_ERROR;
    }

    strcpy(tmp, path);

    /* Strip trailing slashes */
    while (len > 1 && is_sep(tmp[len - 1])) {
        tmp[--len] = '\0';
    }

    p = tmp;
    /* Skip root slash or drive prefix */
    if (isalpha((unsigned char)p[0]) && p[1] == ':') {
        p += 2;
    }
    while (*p && is_sep(*p)) {
        p++;
    }

    while (*p) {
        while (*p && !is_sep(*p)) {
            p++;
        }
        if (*p) {
            *p = '\0';
            if (!cais_fs_exists(tmp)) {
                if (cais_pal_fs_mkdir(tmp, mode, err_buf, err_cap) != CAIS_OK) {
                    return CAIS_ERROR;
                }
            }
            *p = cais_fs_path_separator();
            p++;
            while (*p && is_sep(*p)) p++;
        }
    }

    if (!cais_fs_exists(tmp)) {
        return cais_pal_fs_mkdir(tmp, mode, err_buf, err_cap);
    }
    return CAIS_OK;
}

cais_status_t cais_fs_rmdir(const char *path, char *err_buf, size_t err_cap)
{
    if (path == NULL) return CAIS_ERROR;
    return cais_pal_fs_rmdir(path, err_buf, err_cap);
}

cais_status_t cais_fs_remove(const char *path, char *err_buf, size_t err_cap)
{
    if (path == NULL) return CAIS_ERROR;
    return cais_pal_fs_unlink(path, err_buf, err_cap);
}

cais_status_t cais_fs_remove_all(const char *path, char *err_buf, size_t err_cap)
{
    cais_fs_stat_t st;

    if (path == NULL || path[0] == '\0') {
        cais_set_err(err_buf, err_cap, "path is null or empty");
        return CAIS_ERROR;
    }

    if (cais_pal_fs_stat(path, &st, 0, err_buf, err_cap) != CAIS_OK) {
        return CAIS_ERROR;
    }

    if (st.type == CAIS_FS_TYPE_DIR) {
        cais_fs_dir_t *dir = cais_pal_fs_dir_open(path, err_buf, err_cap);
        cais_fs_dirent_t ent;

        if (dir != NULL) {
            while (cais_pal_fs_dir_next(dir, &ent) == CAIS_OK) {
                char child[CAIS_PATH_MAX];
                if (cais_fs_path_join(path, ent.name, child, sizeof(child)) == CAIS_OK) {
                    if (cais_fs_remove_all(child, err_buf, err_cap) != CAIS_OK) {
                        cais_pal_fs_dir_close(dir);
                        return CAIS_ERROR;
                    }
                }
            }
            cais_pal_fs_dir_close(dir);
        }
        return cais_pal_fs_rmdir(path, err_buf, err_cap);
    } else {
        return cais_pal_fs_unlink(path, err_buf, err_cap);
    }
}

cais_status_t cais_fs_rename(const char *oldpath, const char *newpath, char *err_buf, size_t err_cap)
{
    if (oldpath == NULL || newpath == NULL) {
        cais_set_err(err_buf, err_cap, "path argument is null");
        return CAIS_ERROR;
    }
    return cais_pal_fs_rename(oldpath, newpath, err_buf, err_cap);
}

cais_status_t cais_fs_copy_file(const char *src, const char *dst, cais_bool_t overwrite, char *err_buf, size_t err_cap)
{
    FILE *in, *out;
    char chunk[8192];
    size_t n;

    if (src == NULL || dst == NULL) {
        cais_set_err(err_buf, err_cap, "source or destination path is null");
        return CAIS_ERROR;
    }

    if (!overwrite && cais_fs_exists(dst)) {
        cais_set_err(err_buf, err_cap, "destination file already exists");
        return CAIS_ERROR;
    }

    in = fopen(src, "rb");
    if (in == NULL) {
        cais_set_err_fmt(err_buf, err_cap, "failed to open source '%s'", src);
        return CAIS_ERROR;
    }

    out = fopen(dst, "wb");
    if (out == NULL) {
        fclose(in);
        cais_set_err_fmt(err_buf, err_cap, "failed to open destination '%s'", dst);
        return CAIS_ERROR;
    }

    while ((n = fread(chunk, 1, sizeof(chunk), in)) > 0) {
        if (fwrite(chunk, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            cais_set_err(err_buf, err_cap, "write error during file copy");
            return CAIS_ERROR;
        }
    }

    fclose(in);
    fclose(out);
    return CAIS_OK;
}

cais_status_t cais_fs_chmod(const char *path, int mode, char *err_buf, size_t err_cap)
{
    if (path == NULL) return CAIS_ERROR;
    return cais_pal_fs_chmod(path, mode, err_buf, err_cap);
}

/* =============================================================================
 * 4. Directory Iteration
 * ============================================================================= */

cais_fs_dir_t *cais_fs_dir_open(const char *path, char *err_buf, size_t err_cap)
{
    return cais_pal_fs_dir_open(path, err_buf, err_cap);
}

cais_status_t cais_fs_dir_next(cais_fs_dir_t *dir, cais_fs_dirent_t *ent)
{
    return cais_pal_fs_dir_next(dir, ent);
}

void cais_fs_dir_close(cais_fs_dir_t *dir)
{
    cais_pal_fs_dir_close(dir);
}

/* =============================================================================
 * 5. File Content Operations (Slurp / Spit & Streaming)
 * ============================================================================= */

cais_status_t cais_fs_read_file(const char *path, void **out_buf, size_t *out_len, char *err_buf, size_t err_cap)
{
    FILE *fp;
    long fsize;
    char *buffer;
    size_t read_bytes;

    if (path == NULL || out_buf == NULL || out_len == NULL) {
        cais_set_err(err_buf, err_cap, "invalid null argument to read_file");
        return CAIS_ERROR;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        cais_set_err_fmt(err_buf, err_cap, "failed to open file '%s'", path);
        return CAIS_ERROR;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        cais_set_err(err_buf, err_cap, "failed to seek to file end");
        return CAIS_ERROR;
    }

    fsize = ftell(fp);
    if (fsize < 0) {
        fclose(fp);
        cais_set_err(err_buf, err_cap, "failed to determine file size");
        return CAIS_ERROR;
    }
    fseek(fp, 0, SEEK_SET);

    buffer = (char *)malloc((size_t)fsize + 1);
    if (buffer == NULL) {
        fclose(fp);
        cais_set_err(err_buf, err_cap, "out of memory allocating file read buffer");
        return CAIS_ERROR;
    }

    read_bytes = (fsize > 0) ? fread(buffer, 1, (size_t)fsize, fp) : 0;
    buffer[read_bytes] = '\0';
    fclose(fp);

    *out_buf = buffer;
    *out_len = read_bytes;
    return CAIS_OK;
}

cais_status_t cais_fs_write_file(const char *path, const void *buf, size_t len, char *err_buf, size_t err_cap)
{
    FILE *fp;
    if (path == NULL) {
        cais_set_err(err_buf, err_cap, "path is null");
        return CAIS_ERROR;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        cais_set_err_fmt(err_buf, err_cap, "failed to open file '%s' for writing", path);
        return CAIS_ERROR;
    }

    if (len > 0 && buf != NULL) {
        if (fwrite(buf, 1, len, fp) != len) {
            fclose(fp);
            cais_set_err(err_buf, err_cap, "failed to write all bytes to file");
            return CAIS_ERROR;
        }
    }

    fclose(fp);
    return CAIS_OK;
}

cais_status_t cais_fs_append_file(const char *path, const void *buf, size_t len, char *err_buf, size_t err_cap)
{
    FILE *fp;
    if (path == NULL) {
        cais_set_err(err_buf, err_cap, "path is null");
        return CAIS_ERROR;
    }

    fp = fopen(path, "ab");
    if (fp == NULL) {
        cais_set_err_fmt(err_buf, err_cap, "failed to open file '%s' for appending", path);
        return CAIS_ERROR;
    }

    if (len > 0 && buf != NULL) {
        if (fwrite(buf, 1, len, fp) != len) {
            fclose(fp);
            cais_set_err(err_buf, err_cap, "failed to append all bytes to file");
            return CAIS_ERROR;
        }
    }

    fclose(fp);
    return CAIS_OK;
}

cais_file_t *cais_file_open(const char *path, const char *mode, char *err_buf, size_t err_cap)
{
    cais_file_t *f;
    char bmode[8];
    FILE *fp;

    if (path == NULL || mode == NULL) {
        cais_set_err(err_buf, err_cap, "path or mode is null");
        return NULL;
    }

    /* Convert mode to binary equivalent for consistent cross-platform behavior */
    if (strcmp(mode, "r") == 0) strcpy(bmode, "rb");
    else if (strcmp(mode, "w") == 0) strcpy(bmode, "wb");
    else if (strcmp(mode, "a") == 0) strcpy(bmode, "ab");
    else if (strcmp(mode, "r+") == 0) strcpy(bmode, "rb+");
    else if (strcmp(mode, "w+") == 0) strcpy(bmode, "wb+");
    else if (strcmp(mode, "a+") == 0) strcpy(bmode, "ab+");
    else strncpy(bmode, mode, sizeof(bmode) - 1);
    bmode[sizeof(bmode) - 1] = '\0';

    fp = fopen(path, bmode);
    if (fp == NULL) {
        cais_set_err_fmt(err_buf, err_cap, "failed to open file '%s'", path);
        return NULL;
    }

    f = (cais_file_t *)malloc(sizeof(cais_file_t));
    if (f == NULL) {
        fclose(fp);
        cais_set_err(err_buf, err_cap, "out of memory allocating cais_file_t");
        return NULL;
    }

    f->fp = fp;
    strncpy(f->mode, bmode, sizeof(f->mode) - 1);
    f->mode[sizeof(f->mode) - 1] = '\0';
    return f;
}

cais_status_t cais_file_read(cais_file_t *f, void *buf, size_t count, size_t *bytes_read)
{
    size_t n;
    if (f == NULL || f->fp == NULL || buf == NULL) return CAIS_ERROR;
    if (count == 0) {
        if (bytes_read) *bytes_read = 0;
        return CAIS_OK;
    }

    n = fread(buf, 1, count, f->fp);
    if (bytes_read) *bytes_read = n;

    if (n == 0) {
        if (feof(f->fp)) return CAIS_EOF;
        if (ferror(f->fp)) return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_file_write(cais_file_t *f, const void *buf, size_t count, size_t *bytes_written)
{
    size_t n;
    if (f == NULL || f->fp == NULL) return CAIS_ERROR;
    if (count == 0) {
        if (bytes_written) *bytes_written = 0;
        return CAIS_OK;
    }

    n = fwrite(buf, 1, count, f->fp);
    if (bytes_written) *bytes_written = n;

    if (n < count && ferror(f->fp)) {
        return CAIS_ERROR;
    }
    return CAIS_OK;
}

cais_status_t cais_file_seek(cais_file_t *f, long offset, int whence, long *new_pos)
{
    int w = SEEK_SET;
    if (f == NULL || f->fp == NULL) return CAIS_ERROR;

    if (whence == CAIS_FS_SEEK_SET) w = SEEK_SET;
    else if (whence == CAIS_FS_SEEK_CUR) w = SEEK_CUR;
    else if (whence == CAIS_FS_SEEK_END) w = SEEK_END;
    else return CAIS_ERROR;

    if (fseek(f->fp, offset, w) != 0) {
        return CAIS_ERROR;
    }

    if (new_pos) {
        *new_pos = ftell(f->fp);
    }
    return CAIS_OK;
}

cais_status_t cais_file_tell(cais_file_t *f, long *pos)
{
    long p;
    if (f == NULL || f->fp == NULL || pos == NULL) return CAIS_ERROR;
    p = ftell(f->fp);
    if (p < 0) return CAIS_ERROR;
    *pos = p;
    return CAIS_OK;
}

cais_status_t cais_file_flush(cais_file_t *f)
{
    if (f == NULL || f->fp == NULL) return CAIS_ERROR;
    return (fflush(f->fp) == 0) ? CAIS_OK : CAIS_ERROR;
}

void cais_file_close(cais_file_t *f)
{
    if (f != NULL) {
        if (f->fp != NULL) {
            fclose(f->fp);
            f->fp = NULL;
        }
        free(f);
    }
}

/* =============================================================================
 * 6. Environment & Special Directories
 * ============================================================================= */

cais_status_t cais_fs_getcwd(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    if (buf == NULL || cap == 0) {
        cais_set_err(err_buf, err_cap, "invalid buffer");
        return CAIS_ERROR;
    }
    return cais_pal_fs_getcwd(buf, cap, err_buf, err_cap);
}

cais_status_t cais_fs_setcwd(const char *path, char *err_buf, size_t err_cap)
{
    if (path == NULL || path[0] == '\0') {
        cais_set_err(err_buf, err_cap, "path is null or empty");
        return CAIS_ERROR;
    }
    return cais_pal_fs_setcwd(path, err_buf, err_cap);
}

cais_status_t cais_fs_temp_dir(char *buf, size_t cap, char *err_buf, size_t err_cap)
{
    if (buf == NULL || cap == 0) {
        cais_set_err(err_buf, err_cap, "invalid buffer");
        return CAIS_ERROR;
    }
    return cais_pal_fs_temp_dir(buf, cap, err_buf, err_cap);
}
