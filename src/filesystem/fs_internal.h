/*
 * cais - Physical I/O, System Foundations & Hardware Abstractions
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_FS_INTERNAL_H
#define CAIS_FS_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cais/platform.h>
#include <cais/types.h>
#include <cais/filesystem.h>

#ifndef CAIS_PATH_MAX
#define CAIS_PATH_MAX 1024
#endif

/*
 * File stream handle implementation
 */
struct cais_file {
    FILE *fp;
    char mode[8];
};

/*
 * PAL (Platform Abstraction Layer) SPI Prototypes
 * Implemented across POSIX, Windows, DOS, Amiga, Atari, OS/2, Plan 9, etc.
 */
cais_status_t cais_pal_fs_stat(const char *path, cais_fs_stat_t *st, cais_bool_t follow_links, char *err_buf, size_t err_cap);
cais_status_t cais_pal_fs_mkdir(const char *path, int mode, char *err_buf, size_t err_cap);
cais_status_t cais_pal_fs_rmdir(const char *path, char *err_buf, size_t err_cap);
cais_status_t cais_pal_fs_unlink(const char *path, char *err_buf, size_t err_cap);
cais_status_t cais_pal_fs_rename(const char *oldpath, const char *newpath, char *err_buf, size_t err_cap);
cais_status_t cais_pal_fs_chmod(const char *path, int mode, char *err_buf, size_t err_cap);
cais_fs_dir_t *cais_pal_fs_dir_open(const char *path, char *err_buf, size_t err_cap);
cais_status_t cais_pal_fs_dir_next(cais_fs_dir_t *dir, cais_fs_dirent_t *ent);
void cais_pal_fs_dir_close(cais_fs_dir_t *dir);
cais_status_t cais_pal_fs_getcwd(char *buf, size_t cap, char *err_buf, size_t err_cap);
cais_status_t cais_pal_fs_setcwd(const char *path, char *err_buf, size_t err_cap);
cais_status_t cais_pal_fs_temp_dir(char *buf, size_t cap, char *err_buf, size_t err_cap);

#endif /* CAIS_FS_INTERNAL_H */
