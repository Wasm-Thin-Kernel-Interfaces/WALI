// CMD: args="short"
// CMD: args="zero"
// CMD: args="negative"
// CMD: args="nsec_overflow"

#include "wali_start.c"
#include <time.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_nanosleep")))
long __imported_wali_nanosleep(const struct timespec *req, struct timespec *rem);
int wali_nanosleep(const struct timespec *req, struct timespec *rem) { return (int)__imported_wali_nanosleep(req, rem); }
#else
#include <sys/syscall.h>
int wali_nanosleep(const struct timespec *req, struct timespec *rem) {
#ifdef SYS_nanosleep
    return syscall(SYS_nanosleep, req, rem);
#else
    return syscall(SYS_clock_nanosleep, CLOCK_REALTIME, 0, req, rem);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "short";

    struct timespec req = {0, 0};
    int expect_ok = 1;
    if (!strcmp(mode, "short"))         { req.tv_sec = 0; req.tv_nsec = 1000000; }  // 1ms
    else if (!strcmp(mode, "zero"))     { req.tv_sec = 0; req.tv_nsec = 0; }
    else if (!strcmp(mode, "negative")) { req.tv_sec = -1; req.tv_nsec = 0; expect_ok = 0; }
    else if (!strcmp(mode, "nsec_overflow")) { req.tv_sec = 0; req.tv_nsec = 1000000000; expect_ok = 0; }
    else return -1;

    long r = wali_nanosleep(&req, NULL);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
