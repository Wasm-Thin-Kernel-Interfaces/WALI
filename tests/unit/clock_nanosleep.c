// CMD: args="monotonic_relative"
// CMD: args="realtime_relative"
// CMD: args="boottime_relative"
// CMD: args="absolute_past"
// CMD: args="invalid_clock"

#include "wali_start.c"
#include <string.h>

#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME 1
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "monotonic_relative";

    clockid_t clk = CLOCK_MONOTONIC;
    int flags = 0;
    struct timespec ts = {0, 1000};  // 1us, relative
    int expect_ok = 1;

    if (!strcmp(mode, "monotonic_relative")) {
        // defaults
    } else if (!strcmp(mode, "realtime_relative")) {
        clk = CLOCK_REALTIME;
    } else if (!strcmp(mode, "boottime_relative")) {
        clk = CLOCK_BOOTTIME;
    } else if (!strcmp(mode, "absolute_past")) {
        // Absolute time in the deep past — clock_nanosleep returns 0 immediately.
        flags = TIMER_ABSTIME;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
    } else if (!strcmp(mode, "invalid_clock")) {
        clk = (clockid_t)9999;
        expect_ok = 0;
    } else {
        return -1;
    }

    long r = wali_syscall_clock_nanosleep(clk, flags, &ts, NULL);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
