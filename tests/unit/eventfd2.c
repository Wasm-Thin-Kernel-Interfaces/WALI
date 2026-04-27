// CMD: args="nonblock"
// CMD: args="cloexec"
// CMD: args="semaphore"
// CMD: args="nonblock_empty_read"

#include "wali_start.c"
#include <sys/eventfd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_eventfd2") long wali_syscall_eventfd2(unsigned int initval, int flags);
int wali_eventfd2(unsigned int initval, int flags) { return (int)wali_syscall_eventfd2(initval, flags); }
#else
#include <sys/syscall.h>
int wali_eventfd2(unsigned int initval, int flags) { return syscall(SYS_eventfd2, initval, flags); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "nonblock";

    unsigned int initval = 5;
    int flags = 0;
    if (!strcmp(mode, "nonblock"))             flags = EFD_NONBLOCK;
    else if (!strcmp(mode, "cloexec"))         flags = EFD_CLOEXEC;
    else if (!strcmp(mode, "semaphore"))       { flags = EFD_SEMAPHORE; initval = 3; }
    else if (!strcmp(mode, "nonblock_empty_read")) { flags = EFD_NONBLOCK; initval = 0; }
    else return -1;

    int efd = wali_eventfd2(initval, flags);
    if (efd < 0) return -1;

    int ret = -1;
    uint64_t val;

    if (!strcmp(mode, "nonblock")) {
        if (wali_syscall_read(efd, &val, sizeof(val)) == sizeof(val) && val == 5) ret = 0;
    } else if (!strcmp(mode, "cloexec")) {
        long fdflags = wali_syscall_fcntl(efd, F_GETFD, 0);
        if (fdflags >= 0 && (fdflags & FD_CLOEXEC)) ret = 0;
    } else if (!strcmp(mode, "semaphore")) {
        // Each read returns 1 (semaphore mode).
        if (wali_syscall_read(efd, &val, sizeof(val)) != sizeof(val) || val != 1) goto out;
        if (wali_syscall_read(efd, &val, sizeof(val)) != sizeof(val) || val != 1) goto out;
        ret = 0;
    } else if (!strcmp(mode, "nonblock_empty_read")) {
        // Empty + non-blocking → read fails with EAGAIN.
        if (wali_syscall_read(efd, &val, sizeof(val)) < 0) ret = 0;
    }
out:
    wali_syscall_close(efd);
    return ret;
}
