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

#include <cais/platform.h>
#include <cais/sandbox.h>

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
 * Prioritizes kernel UAPI / libc headers from <sys/syscall.h> and <asm/unistd.h>.
 * Only if __NR_landlock_create_ruleset is still undefined do we apply per-arch constants:
 *   - x86_64, aarch64, riscv: 444, 445, 446
 *   - i386: 435, 436, 437
 *   - arm (EABI): 442, 443, 444
 * Any unverified architecture is left undefined so probe_landlock_abi() returns
 * CAIS_SANDBOX_UNSUPPORTED instead of calling an arbitrary system call.
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
 * Symbolic constants are used instead of opaque magic hex numbers to prevent
 * ABI desynchronization across kernel versions.
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
#else
    unsigned long handled_access_fs;
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

static int s_ruleset_fd = -1;
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

/*
 * Returns the Landlock filesystem access mask supported by the running kernel.
 * Caps at known filesystem flags up to ABI 5, ensuring forward-compatibility
 * when running under newer kernels with ABI >= 6 (which add IPC scoping, etc.).
 */
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

/*
 * Computes Landlock access rights for an unveiled path given permissions and node type.
 */
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

cais_sandbox_backend_t cais_sandbox_backend(int *version_out)
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

const char *cais_sandbox_backend_name(int *version_out)
{
    cais_sandbox_backend_t b = cais_sandbox_backend(version_out);
    return (b == CAIS_SANDBOX_BACKEND_LANDLOCK) ? "landlock" : "none";
}

cais_bool_t cais_sandbox_is_supported(void)
{
    int version = 0;
    return (cais_sandbox_backend(&version) != CAIS_SANDBOX_BACKEND_NONE && version > 0) ? CAIS_TRUE : CAIS_FALSE;
}

cais_sandbox_status_t cais_sandbox_unveil(const char *path, int permissions, char *err_buf, size_t err_cap)
{
    int abi;
    int fd;
    struct stat st;
    struct landlock_path_beneath_attr pb;

    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

    if (path == NULL || path[0] == '\0') {
        return CAIS_SANDBOX_OK;
    }

    abi = probe_landlock_abi();
    if (abi <= 0) {
        cais_set_err(err_buf, err_cap, "Landlock is not supported by the running Linux kernel");
        return CAIS_SANDBOX_UNSUPPORTED;
    }

#if defined(__NR_landlock_create_ruleset)
    if (s_ruleset_fd < 0) {
        struct landlock_ruleset_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.handled_access_fs = get_fs_mask(abi);
        s_ruleset_fd = (int)syscall(__NR_landlock_create_ruleset, &attr, sizeof(attr), 0);
        if (s_ruleset_fd < 0) {
            cais_set_err_fmt(err_buf, err_cap, "landlock_create_ruleset failed (%s)", strerror(errno));
            return CAIS_SANDBOX_ERROR;
        }
    }

    fd = open(path, O_PATH | O_CLOEXEC);
    if (fd < 0) {
        fd = open(path, O_RDONLY);
    }
    if (fd < 0) {
        cais_set_err_fmt(err_buf, err_cap, "Cannot open path '%.128s' for sandbox: %s", path, strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }

    if (fstat(fd, &st) != 0) {
        close(fd);
        cais_set_err(err_buf, err_cap, "fstat failed on sandbox target");
        return CAIS_SANDBOX_ERROR;
    }

    memset(&pb, 0, sizeof(pb));
    pb.allowed_access = compute_access_rights(abi, permissions, S_ISDIR(st.st_mode));
    pb.parent_fd = fd;

    if (syscall(__NR_landlock_add_rule, s_ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &pb, 0) != 0) {
        close(fd);
        cais_set_err_fmt(err_buf, err_cap, "landlock_add_rule failed on '%.128s': %s", path, strerror(errno));
        return CAIS_SANDBOX_ERROR;
    }

    close(fd);
    return CAIS_SANDBOX_OK;
#else
    (void)path;
    (void)permissions;
    cais_set_err(err_buf, err_cap, "Landlock is not supported in this build");
    return CAIS_SANDBOX_UNSUPPORTED;
#endif
}

cais_sandbox_status_t cais_sandbox_lock(char *err_buf, size_t err_cap)
{
    if (err_buf != NULL && err_cap > 0) {
        err_buf[0] = '\0';
    }

#if defined(__NR_landlock_restrict_self)
    if (s_ruleset_fd >= 0) {
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
            cais_set_err(err_buf, err_cap, "prctl(PR_SET_NO_NEW_PRIVS) failed");
            return CAIS_SANDBOX_ERROR;
        }
        if (syscall(__NR_landlock_restrict_self, s_ruleset_fd, 0) != 0) {
            cais_set_err_fmt(err_buf, err_cap, "landlock_restrict_self failed (%s)", strerror(errno));
            close(s_ruleset_fd);
            s_ruleset_fd = -1;
            return CAIS_SANDBOX_ERROR;
        }
        close(s_ruleset_fd);
        s_ruleset_fd = -1;
        return CAIS_SANDBOX_OK;
    } else {
        /*
         * When lock is called without prior unveil(), create an empty ruleset
         * and enforce it so that all handled filesystem access is locked down.
         */
        int abi = probe_landlock_abi();
        int empty_fd;
        struct landlock_ruleset_attr attr;

        if (abi <= 0) {
            cais_set_err(err_buf, err_cap, "Landlock is not supported by the running Linux kernel");
            return CAIS_SANDBOX_UNSUPPORTED;
        }

        memset(&attr, 0, sizeof(attr));
        attr.handled_access_fs = get_fs_mask(abi);
        empty_fd = (int)syscall(__NR_landlock_create_ruleset, &attr, sizeof(attr), 0);
        if (empty_fd < 0) {
            cais_set_err_fmt(err_buf, err_cap, "landlock_create_ruleset failed (%s)", strerror(errno));
            return CAIS_SANDBOX_ERROR;
        }

        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
            cais_set_err(err_buf, err_cap, "prctl(PR_SET_NO_NEW_PRIVS) failed");
            close(empty_fd);
            return CAIS_SANDBOX_ERROR;
        }

        if (syscall(__NR_landlock_restrict_self, empty_fd, 0) != 0) {
            cais_set_err_fmt(err_buf, err_cap, "landlock_restrict_self failed (%s)", strerror(errno));
            close(empty_fd);
            return CAIS_SANDBOX_ERROR;
        }

        close(empty_fd);
        return CAIS_SANDBOX_OK;
    }
#else
    cais_set_err(err_buf, err_cap, "Landlock is not supported in this build");
    return CAIS_SANDBOX_UNSUPPORTED;
#endif
}

cais_sandbox_status_t cais_sandbox_pledge(const char *promises, char *err_buf, size_t err_cap)
{
    (void)promises;
    cais_set_err(err_buf, err_cap, "pledge is not supported on Linux (Landlock only supports filesystem restrictions)");
    return CAIS_SANDBOX_UNSUPPORTED;
}

#endif /* CAIS_IS_LINUX || defined(__linux__) */
