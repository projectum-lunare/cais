/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 * Mozilla Public License Version 2.0
 */

#include "test_common.h"

/*
 * ============================================================================
 * Kernel Security Sandbox & Process Confinement Tests
 * ============================================================================
 * Cais provides portable declarative sandboxing using native kernel mechanisms:
 * - Linux: Landlock LSM
 * - Apple Darwin (macOS): Seatbelt (sandbox_init)
 * - Windows NT: Process Mitigation Policies & Token Hardening
 * - OpenBSD: pledge() & unveil()
 * - FreeBSD: Capsicum capability mode
 *
 * NOTE ON TESTING ISOLATION:
 * Sandbox restrictions are irreversible within a process. Applying a sandbox
 * directly to the main test process would restrict subsequent tests. Therefore,
 * live confinement tests use fork() to isolate sandbox enforcement within a child
 * process, leaving the test runner parent unrestricted.
 */

/*
 * Verifies that the sandbox subsystem correctly detects the host kernel's
 * active sandboxing mechanism and ABI/protocol version.
 */
TEST test_sandbox(void)
{
    int version = 0;
    cais_sandbox_backend_t b = cais_sandbox_backend(&version);
    const char *backend = cais_sandbox_backend_name(&version);

    /* The backend name must be a valid, non-empty descriptive string (e.g. "landlock") */
    ASSERT(backend != NULL);
    ASSERT(strlen(backend) > 0);

    /* Test predicate function cais_sandbox_is_supported() */
    if (b != CAIS_SANDBOX_BACKEND_NONE) {
        ASSERT_EQ(CAIS_TRUE, cais_sandbox_is_supported());
    } else {
        ASSERT_EQ(CAIS_FALSE, cais_sandbox_is_supported());
    }

    /* The backend enum must fall within valid known backend ranges */
    ASSERT(b >= CAIS_SANDBOX_BACKEND_NONE && b <= CAIS_SANDBOX_BACKEND_WIN32);

    PASS();
}

/*
 * Verifies the binary values and bitmask semantics of sandbox constants
 * defined in cais/sandbox.h to ensure ABI stability.
 */
TEST test_sandbox_enums(void)
{
    /* Backend identifiers */
    ASSERT_EQ(0, CAIS_SANDBOX_BACKEND_NONE);
    ASSERT_EQ(1, CAIS_SANDBOX_BACKEND_LANDLOCK);
    ASSERT_EQ(2, CAIS_SANDBOX_BACKEND_OPENBSD);
    ASSERT_EQ(3, CAIS_SANDBOX_BACKEND_CAPSICUM);
    ASSERT_EQ(4, CAIS_SANDBOX_BACKEND_SEATBELT);
    ASSERT_EQ(5, CAIS_SANDBOX_BACKEND_WIN32);

    /* Filesystem permission bitmasks */
    ASSERT_EQ(0, CAIS_SANDBOX_PERM_NONE);
    ASSERT_EQ(1 << 0, CAIS_SANDBOX_PERM_READ);
    ASSERT_EQ(1 << 1, CAIS_SANDBOX_PERM_WRITE);
    ASSERT_EQ(1 << 2, CAIS_SANDBOX_PERM_EXEC);

    /* Network policy levels */
    ASSERT_EQ(0, CAIS_SANDBOX_NET_DEFAULT);
    ASSERT_EQ(1, CAIS_SANDBOX_NET_NONE);
    ASSERT_EQ(2, CAIS_SANDBOX_NET_LOOPBACK);
    ASSERT_EQ(3, CAIS_SANDBOX_NET_ALL);

    /* Status return codes */
    ASSERT_EQ(0, CAIS_SANDBOX_OK);
    ASSERT_EQ(-1, CAIS_SANDBOX_ERROR);
    ASSERT_EQ(-4, CAIS_SANDBOX_UNSUPPORTED);

    PASS();
}

/*
 * Verifies the declarative sandbox policy lifecycle and configuration methods.
 */
TEST test_sandbox_policy_lifecycle(void)
{
    cais_sandbox_policy_t *policy = cais_sandbox_policy_create();
    ASSERT(policy != NULL);

    ASSERT_EQ(CAIS_OK, cais_sandbox_policy_allow_path(policy, ".", CAIS_SANDBOX_READ));
    ASSERT_EQ(CAIS_OK, cais_sandbox_policy_allow_path(policy, "/tmp", CAIS_SANDBOX_READ | CAIS_SANDBOX_WRITE));
    ASSERT_EQ(CAIS_OK, cais_sandbox_policy_set_network(policy, CAIS_SANDBOX_NET_NONE));
    ASSERT_EQ(CAIS_OK, cais_sandbox_policy_set_allow_child_processes(policy, CAIS_FALSE));
    ASSERT_EQ(CAIS_OK, cais_sandbox_policy_set_allow_dynamic_code(policy, CAIS_FALSE));

    /* Null validation */
    ASSERT_EQ(CAIS_ERROR, cais_sandbox_policy_allow_path(NULL, ".", CAIS_SANDBOX_READ));
    ASSERT_EQ(CAIS_ERROR, cais_sandbox_policy_allow_path(policy, NULL, CAIS_SANDBOX_READ));
    ASSERT_EQ(CAIS_ERROR, cais_sandbox_policy_set_network(NULL, CAIS_SANDBOX_NET_NONE));

    cais_sandbox_policy_destroy(policy);
    PASS();
}

/*
 * Verifies that applying a policy with a nonexistent path fails gracefully
 * on platforms that validate paths (like Linux Landlock).
 */
TEST test_sandbox_invalid_path(void)
{
#if CAIS_IS_LINUX
    int version = 0;
    cais_sandbox_backend_t b = cais_sandbox_backend(&version);

    if (b == CAIS_SANDBOX_BACKEND_LANDLOCK && version > 0) {
        char err_buf[256];
        cais_sandbox_status_t rc;
        cais_sandbox_policy_t *p = cais_sandbox_policy_create();
        ASSERT(p != NULL);

        cais_sandbox_policy_allow_path(p, "/nonexistent_sandbox_path_12345", CAIS_SANDBOX_READ);
        err_buf[0] = '\0';
        rc = cais_sandbox_apply(p, err_buf, sizeof(err_buf));
        cais_sandbox_policy_destroy(p);

        ASSERT_EQ(CAIS_SANDBOX_ERROR, rc);
        ASSERT(strlen(err_buf) > 0);
    }
#endif
    PASS();
}

/*
 * Tests active filesystem confinement using Landlock on Linux.
 *
 * SCENARIO:
 * 1. Forks a child process.
 * 2. In the child, creates a policy allowing "." with READ permission.
 * 3. Enforces the sandbox via cais_sandbox_apply(policy).
 * 4. Verifies that reading a file inside "." (e.g. "xmake.lua") succeeds.
 * 5. Verifies that opening an external system file (e.g. "/etc/passwd") is
 *    denied by the kernel (Landlock returns EACCES / NULL).
 * 6. Parent waits for child and asserts exit code 0.
 */
TEST test_sandbox_fork(void)
{
#if CAIS_IS_LINUX
    int version = 0;
    int status = 0;
    cais_sandbox_backend_t b = cais_sandbox_backend(&version);

    /* Only run live enforcement if Landlock is actively supported by the host kernel */
    if (b == CAIS_SANDBOX_BACKEND_LANDLOCK && version > 0) {
        pid_t pid = fork();
        ASSERT(pid >= 0);

        if (pid == 0) {
            /* --- CHILD PROCESS --- */
            char err_buf[128];
            FILE *f;
            cais_sandbox_status_t rc;
            cais_sandbox_policy_t *p = cais_sandbox_policy_create();
            if (!p) {
                _exit(1);
            }

            cais_sandbox_policy_allow_path(p, ".", CAIS_SANDBOX_PERM_READ);
            err_buf[0] = '\0';
            rc = cais_sandbox_apply(p, err_buf, sizeof(err_buf));
            cais_sandbox_policy_destroy(p);

            if (rc != CAIS_SANDBOX_OK) {
                _exit(2);
            }

            /* Permitted read: local file in allowed path must succeed */
            f = fopen("xmake.lua", "r");
            if (!f) {
                _exit(3);
            }
            fclose(f);

            /* Forbidden read: file outside allowed hierarchy must be blocked by Landlock */
            f = fopen("/etc/passwd", "r");
            if (f) {
                fclose(f);
                _exit(4); /* Security breach: file was opened despite sandbox! */
            }

            _exit(0); /* Success: sandbox behavior matches security policy */
        }

        /* --- PARENT PROCESS --- */
        status = 0;
        waitpid(pid, &status, 0);
        ASSERT(WIFEXITED(status));
        ASSERT_EQ(0, WEXITSTATUS(status));
    }
#endif

    PASS();
}

/*
 * Tests complete filesystem lockout when applying NULL policy (default lockdown).
 *
 * SCENARIO:
 * 1. Forks a child process.
 * 2. In the child, invokes cais_sandbox_apply(NULL).
 * 3. Under Landlock rules, an empty ruleset blocks all new filesystem openings.
 * 4. Verifies that opening "/etc/passwd" is denied.
 * 5. Parent verifies child exits cleanly with code 0.
 */
TEST test_sandbox_lock_without_unveil_fork(void)
{
#if CAIS_IS_LINUX
    int version = 0;
    int status = 0;
    cais_sandbox_backend_t b = cais_sandbox_backend(&version);

    if (b == CAIS_SANDBOX_BACKEND_LANDLOCK && version > 0) {
        pid_t pid = fork();
        ASSERT(pid >= 0);

        if (pid == 0) {
            /* --- CHILD PROCESS --- */
            char err_buf[128];
            FILE *f;
            cais_sandbox_status_t rc;

            err_buf[0] = '\0';
            rc = cais_sandbox_apply(NULL, err_buf, sizeof(err_buf));
            if (rc != CAIS_SANDBOX_OK) {
                _exit(1);
            }

            /* Any filesystem open attempt must be blocked */
            f = fopen("/etc/passwd", "r");
            if (f) {
                fclose(f);
                _exit(2); /* Failure: access was not blocked */
            }

            _exit(0); /* Success */
        }

        /* --- PARENT PROCESS --- */
        status = 0;
        waitpid(pid, &status, 0);
        ASSERT(WIFEXITED(status));
        ASSERT_EQ(0, WEXITSTATUS(status));
    }
#endif

    PASS();
}

SUITE(sandbox_suite)
{
    RUN_TEST(test_sandbox);
    RUN_TEST(test_sandbox_enums);
    RUN_TEST(test_sandbox_policy_lifecycle);
    RUN_TEST(test_sandbox_invalid_path);
    RUN_TEST(test_sandbox_fork);
    RUN_TEST(test_sandbox_lock_without_unveil_fork);
}
