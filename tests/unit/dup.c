// CMD: args="ok"
// CMD: args="bad_fd"
// CMD: args="negative"

#include "wali_start.c"
#include <string.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    int target_fd;
    int expect_ok;
    if (!strcmp(mode, "ok"))            { target_fd = 0;     expect_ok = 1; }
    else if (!strcmp(mode, "bad_fd"))   { target_fd = 99999; expect_ok = 0; }
    else if (!strcmp(mode, "negative")) { target_fd = -1;    expect_ok = 0; }
    else return -1;

    long r = wali_syscall_dup(target_fd);
    int success = (r >= 0);
    if (success != expect_ok) return -1;
    if (success) {
        if (r == target_fd) return -1;  // dup must return a distinct fd
        wali_syscall_close((int)r);
    }
    return 0;
}
