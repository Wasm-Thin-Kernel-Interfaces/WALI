// CMD: args="zero"
// CMD: args="cloexec"
// CMD: args="invalid"

#include "wali_start.c"
#include <sys/epoll.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

#ifdef __wasm__
WALI_IMPORT("SYS_epoll_create1") long wali_syscall_epoll_create1(int flags);
int wali_epoll_create1(int flags) { return (int)wali_syscall_epoll_create1(flags); }
#else
#include <sys/syscall.h>
int wali_epoll_create1(int flags) { return syscall(SYS_epoll_create1, flags); }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "zero";

    int flags;
    int expect_ok = 1;
    if (!strcmp(mode, "zero"))         flags = 0;
    else if (!strcmp(mode, "cloexec")) flags = EPOLL_CLOEXEC;
    else if (!strcmp(mode, "invalid")) { flags = 0xDEADBEEF; expect_ok = 0; }
    else return -1;

    int r = wali_epoll_create1(flags);
    int success = (r >= 0);
    if (success != expect_ok) return -1;
    if (success) wali_syscall_close(r);
    return 0;
}
