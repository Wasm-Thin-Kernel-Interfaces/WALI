// CMD: args="ok"
// CMD: args="self"
// CMD: args="bad_oldfd"
// CMD: args="negative_newfd"

#include "wali_start.c"
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_dup2") long wali_syscall_dup2(int oldfd, int newfd);
#else
#include <sys/syscall.h>
long wali_syscall_dup2(int oldfd, int newfd) {
#ifdef SYS_dup2
    return syscall(SYS_dup2, oldfd, newfd);
#else
    if (oldfd == newfd) {
        return newfd;  // dup2 with oldfd==newfd is a no-op that returns newfd
    } else {
        return syscall(SYS_dup3, oldfd, newfd, 0);
    }
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    int oldfd, newfd;
    int expect_ok;
    int expect_returned_fd = -1;
    if (!strcmp(mode, "ok"))             { oldfd = 0;     newfd = 10; expect_ok = 1; expect_returned_fd = 10; }
    else if (!strcmp(mode, "self"))      { oldfd = 0;     newfd = 0;  expect_ok = 1; expect_returned_fd = 0; }
    else if (!strcmp(mode, "bad_oldfd")) { oldfd = 99999; newfd = 10; expect_ok = 0; }
    else if (!strcmp(mode, "negative_newfd")) { oldfd = 0; newfd = -1; expect_ok = 0; }
    else return -1;

    long r = wali_syscall_dup2(oldfd, newfd);
    int success = (r >= 0);
    if (success != expect_ok) return -1;
    if (success) {
        if (r != expect_returned_fd) return -1;
        if ((int)r != oldfd) wali_syscall_close((int)r);
    }
    return 0;
}
