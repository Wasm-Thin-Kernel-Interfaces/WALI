// CMD: args="realtime"
// CMD: args="monotonic"
// CMD: args="boottime"
// CMD: args="process_cputime"
// CMD: args="thread_cputime"
// CMD: args="invalid"

#include "wali_start.c"
#include <string.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "realtime";

    clockid_t clk;
    int expect_ok = 1;
    if (!strcmp(mode, "realtime"))             clk = CLOCK_REALTIME;
    else if (!strcmp(mode, "monotonic"))       clk = CLOCK_MONOTONIC;
    else if (!strcmp(mode, "boottime"))        clk = CLOCK_BOOTTIME;
    else if (!strcmp(mode, "process_cputime")) clk = CLOCK_PROCESS_CPUTIME_ID;
    else if (!strcmp(mode, "thread_cputime"))  clk = CLOCK_THREAD_CPUTIME_ID;
    else if (!strcmp(mode, "invalid"))         { clk = (clockid_t)9999; expect_ok = 0; }
    else return -1;

    struct timespec ts;
    long r = wali_syscall_clock_gettime(clk, &ts);
    int success = (r == 0);
    if (success != expect_ok) return -1;
    if (expect_ok) {
        if (ts.tv_sec < 0 || ts.tv_nsec < 0 || ts.tv_nsec >= 1000000000) return -1;
    }
    return 0;
}
