// CMD: args="cloexec"
// CMD: args="none"
// CMD: args="same_fd"
// CMD: args="bad_oldfd"
// CMD: args="bad_flags"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_dup3") long wali_syscall_dup3(int oldfd, int newfd, int flags);
#else
#include <sys/syscall.h>
long wali_syscall_dup3(int oldfd, int newfd, int flags) { return syscall(SYS_dup3, oldfd, newfd, flags); }
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif
#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "cloexec";

    int oldfd = 0, newfd = 20, flags = 0;
    int expect_ok = 1;
    int expect_cloexec = 0;
    if (!strcmp(mode, "cloexec"))   { flags = O_CLOEXEC; expect_cloexec = 1; }
    else if (!strcmp(mode, "none")) { flags = 0; }
    else if (!strcmp(mode, "same_fd"))   { newfd = oldfd; expect_ok = 0; }  // dup3 disallows oldfd==newfd
    else if (!strcmp(mode, "bad_oldfd")) { oldfd = 99999; expect_ok = 0; }
    else if (!strcmp(mode, "bad_flags")) { flags = 0xDEADBEEF & ~O_CLOEXEC; expect_ok = 0; }
    else return -1;

    long r = wali_syscall_dup3(oldfd, newfd, flags);
    int success = (r >= 0);
    if (success != expect_ok) return -1;
    if (!success) return 0;

    if (r != newfd) return -1;
    long fdflags = wali_syscall_fcntl((int)r, F_GETFD, 0);
    if (fdflags < 0) { wali_syscall_close((int)r); return -1; }
    int has_cloexec = (fdflags & FD_CLOEXEC) ? 1 : 0;
    wali_syscall_close((int)r);
    return (has_cloexec == expect_cloexec) ? 0 : -1;
}
