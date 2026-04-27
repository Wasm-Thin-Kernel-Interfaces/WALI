// CMD: args="initval_zero"
// CMD: args="initval_ten"
// CMD: args="read_value"
// CMD: args="write_then_read"

#include "wali_start.c"
#include <stdint.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_eventfd") long wali_syscall_eventfd(unsigned int initval);
int wali_eventfd(unsigned int initval) { return (int)wali_syscall_eventfd(initval); }
#else
#include <sys/syscall.h>
int wali_eventfd(unsigned int initval) {
#ifdef SYS_eventfd
    return syscall(SYS_eventfd, initval);
#else
    return syscall(SYS_eventfd2, initval, 0);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "initval_zero";

    unsigned int initval = (!strcmp(mode, "initval_zero")) ? 0 : 10;
    int efd = wali_eventfd(initval);
    if (efd < 0) return -1;

    int ret = -1;
    uint64_t val;
    if (!strcmp(mode, "initval_zero") || !strcmp(mode, "initval_ten")) {
        ret = 0;
    } else if (!strcmp(mode, "read_value")) {
        if (wali_syscall_read(efd, &val, sizeof(val)) == sizeof(val) && val == 10) ret = 0;
    } else if (!strcmp(mode, "write_then_read")) {
        // Drain initial 10, write 5, read 5.
        if (wali_syscall_read(efd, &val, sizeof(val)) != sizeof(val)) goto out;
        val = 5;
        if (wali_syscall_write(efd, &val, sizeof(val)) != sizeof(val)) goto out;
        if (wali_syscall_read(efd, &val, sizeof(val)) == sizeof(val) && val == 5) ret = 0;
    }
out:
    wali_syscall_close(efd);
    return ret;
}
