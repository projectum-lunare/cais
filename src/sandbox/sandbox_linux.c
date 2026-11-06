/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "sandbox_internal.h"

#if CAIS_IS_LINUX || defined(__linux__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#if defined(__has_include)
#if __has_include(<asm/unistd.h>)
#include <asm/unistd.h>
#endif
#else
#include <asm/unistd.h>
#endif
#include <sys/prctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Landlock system call numbers fallback definition.
 */
#ifndef __NR_landlock_create_ruleset
#if defined(__x86_64__) || defined(__aarch64__) || defined(__riscv)
#define __NR_landlock_create_ruleset 444
#define __NR_landlock_add_rule       445
#define __NR_landlock_restrict_self  446
#elif defined(__i386__)
#define __NR_landlock_create_ruleset 435
#define __NR_landlock_add_rule       436
#define __NR_landlock_restrict_self  437
#elif defined(__arm__)
#define __NR_landlock_create_ruleset 442
#define __NR_landlock_add_rule       443
#define __NR_landlock_restrict_self  444
#endif
#endif

#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#endif

#ifndef O_PATH
#define O_PATH 010000000
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif

#define LANDLOCK_CREATE_RULESET_VERSION (1U << 0)
#define LANDLOCK_RULE_PATH_BENEATH      1

/*
 * Landlock access rights flags (Linux kernel UAPI).
 */
#define CAIS_LANDLOCK_ACCESS_FS_EXECUTE     (1UL << 0)
#define CAIS_LANDLOCK_ACCESS_FS_WRITE_FILE  (1UL << 1)
#define CAIS_LANDLOCK_ACCESS_FS_READ_FILE   (1UL << 2)
#define CAIS_LANDLOCK_ACCESS_FS_READ_DIR    (1UL << 3)
#define CAIS_LANDLOCK_ACCESS_FS_REMOVE_DIR  (1UL << 4)
#define CAIS_LANDLOCK_ACCESS_FS_REMOVE_FILE (1UL << 5)
#define CAIS_LANDLOCK_ACCESS_FS_MAKE_CHAR   (1UL << 6)
#define CAIS_LANDLOCK_ACCESS_FS_MAKE_DIR    (1UL << 7)
#define CAIS_LANDLOCK_ACCESS_FS_MAKE_REG    (1UL << 8)
#define CAIS_LANDLOCK_ACCESS_FS_MAKE_SOCK   (1UL << 9)
#define CAIS_LANDLOCK_ACCESS_FS_MAKE_FIFO   (1UL << 10)
#define CAIS_LANDLOCK_ACCESS_FS_MAKE_BLOCK  (1UL << 11)
#define CAIS_LANDLOCK_ACCESS_FS_MAKE_SYM    (1UL << 12)
#define CAIS_LANDLOCK_ACCESS_FS_REFER       (1UL << 13) /* ABI >= 2 */
#define CAIS_LANDLOCK_ACCESS_FS_TRUNCATE    (1UL << 14) /* ABI >= 3 */
#define CAIS_LANDLOCK_ACCESS_FS_IOCTL_DEV   (1UL << 15) /* ABI >= 5 */

/* Landlock ABI 4+ Network Flags */
#define CAIS_LANDLOCK_ACCESS_NET_BIND_TCP    (1UL << 0)
#define CAIS_LANDLOCK_ACCESS_NET_CONNECT_TCP (1UL << 1)

#define CAIS_LANDLOCK_FS_MASK_V1 ( \
    CAIS_LANDLOCK_ACCESS_FS_EXECUTE | \
    CAIS_LANDLOCK_ACCESS_FS_WRITE_FILE | \
    CAIS_LANDLOCK_ACCESS_FS_READ_FILE | \
    CAIS_LANDLOCK_ACCESS_FS_READ_DIR | \
    CAIS_LANDLOCK_ACCESS_FS_REMOVE_DIR | \
    CAIS_LANDLOCK_ACCESS_FS_REMOVE_FILE | \
    CAIS_LANDLOCK_ACCESS_FS_MAKE_CHAR | \
    CAIS_LANDLOCK_ACCESS_FS_MAKE_DIR | \
    CAIS_LANDLOCK_ACCESS_FS_MAKE_REG | \
    CAIS_LANDLOCK_ACCESS_FS_MAKE_SOCK | \
    CAIS_LANDLOCK_ACCESS_FS_MAKE_FIFO | \
    CAIS_LANDLOCK_ACCESS_FS_MAKE_BLOCK | \
    CAIS_LANDLOCK_ACCESS_FS_MAKE_SYM)

struct landlock_ruleset_attr {
#if defined(__GNUC__)
    __extension__ unsigned long long handled_access_fs;
    __extension__ unsigned long long handled_access_net;
#else
    unsigned long handled_access_fs;
    unsigned long handled_access_net;
#endif
};

struct landlock_path_beneath_attr {
#if defined(__GNUC__)
    __extension__ unsigned long long allowed_access;
#else
    unsigned long allowed_access;
#endif
    int parent_fd;
};

static int s_cached_abi = -2;

static int probe_landlock_abi(void)
{
#if defined(__NR_landlock_create_ruleset)
    long res;
    if (s_cached_abi != -2) {
        return s_cached_abi;
    }
    res = syscall(__NR_landlock_create_ruleset, NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
    if (res > 0) {
        s_cached_abi = (int)res;
    } else {
        s_cached_abi = -1;
    }
    return s_cached_abi;
#else
    s_cached_abi = -1;
    return -1;
#endif
}

static unsigned long get_fs_mask(int abi)
{
    unsigned long mask = CAIS_LANDLOCK_FS_MASK_V1;

    if (abi >= 2) {
        mask |= CAIS_LANDLOCK_ACCESS_FS_REFER;
    }
    if (abi >= 3) {
        mask |= CAIS_LANDLOCK_ACCESS_FS_TRUNCATE;
    }
    if (abi >= 5) {
        mask |= CAIS_LANDLOCK_ACCESS_FS_IOCTL_DEV;
    }
    return mask;
}

static unsigned long compute_access_rights(int abi, int permissions, int is_dir)
{
    unsigned long rights = 0;

    if (permissions & CAIS_SANDBOX_PERM_READ) {
        rights |= CAIS_LANDLOCK_ACCESS_FS_READ_FILE;
        if (is_dir) {
            rights |= CAIS_LANDLOCK_ACCESS_FS_READ_DIR;
        }
    }
    if (permissions & CAIS_SANDBOX_PERM_WRITE) {
        rights |= CAIS_LANDLOCK_ACCESS_FS_WRITE_FILE;
        if (abi >= 3) {
            rights |= CAIS_LANDLOCK_ACCESS_FS_TRUNCATE;
        }
        if (abi >= 5 && !is_dir) {
            rights |= CAIS_LANDLOCK_ACCESS_FS_IOCTL_DEV;
        }
        if (is_dir) {
            rights |= CAIS_LANDLOCK_ACCESS_FS_REMOVE_DIR
                   |  CAIS_LANDLOCK_ACCESS_FS_REMOVE_FILE
                   |  CAIS_LANDLOCK_ACCESS_FS_MAKE_DIR
                   |  CAIS_LANDLOCK_ACCESS_FS_MAKE_REG
                   |  CAIS_LANDLOCK_ACCESS_FS_MAKE_SOCK
                   |  CAIS_LANDLOCK_ACCESS_FS_MAKE_FIFO
                   |  CAIS_LANDLOCK_ACCESS_FS_MAKE_BLOCK
                   |  CAIS_LANDLOCK_ACCESS_FS_MAKE_SYM;
            if (abi >= 2) {
                rights |= CAIS_LANDLOCK_ACCESS_FS_REFER;
            }
        }
    }
    if (permissions & CAIS_SANDBOX_PERM_EXEC) {
        rights |= CAIS_LANDLOCK_ACCESS_FS_EXECUTE;
    }

    return rights & get_fs_mask(abi);
}

cais_sandbox_backend_t cais_pal_sandbox_backend(int *version_out)
{
    int abi = probe_landlock_abi();
    if (abi > 0) {
        if (version_out != NULL) {
            *version_out = abi;
        }
        return CAIS_SANDBOX_BACKEND_LANDLOCK;
    }
    if (version_out != NULL) {
        *version_out = 0;
    }
    return CAIS_SANDBOX_BACKEND_NONE;
}

const char *cais_pal_sandbox_backend_name(int *version_out)
{
    cais_sandbox_backend_t b = cais_pal_sandbox_backend(version_out);
    return (b == CAIS_SANDBOX_BACKEND_LANDLOCK) ? "landlock" : "none";
}

cais_bool_t cais_pal_sandbox_is_supported(void)
{
    int version = 0;
    return (cais_pal_sandbox_backend(&version) != CAIS_SANDBOX_BACKEND_NONE && version > 0) ? CAIS_TRUE : CAIS_FALSE;
}

cais_sandbox_status_t cais_pal_sandbox_apply(const cais_sandbox_policy_t *policy, char *err_buf, size_t err_cap)
{
#if defined(__NR_landlock_create_ruleset) && defined(__NR_landlock_restrict_self)
    int abi;
    int ruleset_fd;
    struct landlock_ruleset_attr attr;
    size_t path_count = 0;
    size_t i;

    abi = probe_landlock_abi();
    if (abi <= 0) {
        cais_set_err(err_buf, err_cap, "Landlock is not supported by the running Linux kernel");
        return CAIS_SANDBOX_UNSUPPORTED;
    }

    memset(&attr, 0, sizeof(attr));
    attr.handled_access_fs = get_fs_mask(abi);

    if (abi >= 4 && policy != NULL && cais_sandbox_policy_get_network(policy) == CAIS_SANDBOX_NET_NONE) {
        attr.handled_access_net = CAIS_LANDLOCK_ACCESS_NET_BIND_TCP | CAIS_LANDLOCK_ACCESS_NET_CONNECT_TCP;
    }

    ruleset_fd = (int)syscall(__NR_landlock_create_ruleset, &attr, sizeof(attr), 0);
    if (ruleset_fd < 0) {
        cais_set_err_fmt(err_buf, err_cap, "landlock_create_ruleset failed: %s", strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }

    if (policy != NULL) {
        path_count = cais_sandbox_policy_path_count(policy);
        for (i = 0; i < path_count; i++) {
            int perms = 0;
            const char *path = cais_sandbox_policy_get_path(policy, i, &perms);
            int fd;
            struct stat st;
            struct landlock_path_beneath_attr pb;

            if (path == NULL || path[0] == '\0') {
                continue;
            }

            fd = open(path, O_PATH | O_CLOEXEC);
            if (fd < 0) {
                fd = open(path, O_RDONLY);
            }
            if (fd < 0) {
                close(ruleset_fd);
                cais_set_err_fmt(err_buf, err_cap, "cannot open path '%.128s' for sandbox: %s", path, strerror(errno));
                return CAIS_SANDBOX_ERROR;
            }

            if (fstat(fd, &st) != 0) {
                close(fd);
                close(ruleset_fd);
                cais_set_err_fmt(err_buf, err_cap, "fstat failed on sandbox target '%.128s'", path);
                return CAIS_SANDBOX_ERROR;
            }

            memset(&pb, 0, sizeof(pb));
            pb.allowed_access = compute_access_rights(abi, perms, S_ISDIR(st.st_mode));
            pb.parent_fd = fd;

            if (syscall(__NR_landlock_add_rule, ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &pb, 0) != 0) {
                close(fd);
                close(ruleset_fd);
                cais_set_err_fmt(err_buf, err_cap, "landlock_add_rule failed on '%.128s': %s", path, strerror(errno));
                return CAIS_SANDBOX_ERROR;
            }

            close(fd);
        }
    }

    /* Enforce NO_NEW_PRIVS */
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        close(ruleset_fd);
        cais_set_err(err_buf, err_cap, "prctl(PR_SET_NO_NEW_PRIVS) failed");
        return CAIS_SANDBOX_ERROR;
    }

    /* Restrict current process and descendants */
    if (syscall(__NR_landlock_restrict_self, ruleset_fd, 0) != 0) {
        close(ruleset_fd);
        cais_set_err_fmt(err_buf, err_cap, "landlock_restrict_self failed: %s", strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }

    close(ruleset_fd);
    return CAIS_SANDBOX_OK;
#else
    (void)policy;
    cais_set_err(err_buf, err_cap, "Landlock is not supported in this build");
    return CAIS_SANDBOX_UNSUPPORTED;
#endif
}

#endif /* CAIS_IS_LINUX || defined(__linux__) */
