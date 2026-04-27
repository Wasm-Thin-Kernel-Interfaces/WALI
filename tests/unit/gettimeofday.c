// CMD: args="basic"
// CMD: args="monotonic"

#include "wali_start.c"
#include <sys/time.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_gettimeofday")))
long __imported_wali_gettimeofday(struct timeval *tv, struct timezone *tz);
int wali_gettimeofday(struct timeval *tv, struct timezone *tz) { return (int)__imported_wali_gettimeofday(tv, tz); }
#else
#include <sys/syscall.h>
int wali_gettimeofday(struct timeval *tv, struct timezone *tz) { return syscall(SYS_gettimeofday, tv, tz); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    if (!strcmp(mode, "basic")) {
        struct timeval tv = {0};
        if (wali_gettimeofday(&tv, NULL) != 0) return -1;
        // Sanity: well past epoch (year > 2010 → tv_sec > 1262304000) and tv_usec in range.
        if (tv.tv_sec < 1262304000) return -1;
        if (tv.tv_usec < 0 || tv.tv_usec >= 1000000) return -1;
        return 0;
    }
    if (!strcmp(mode, "monotonic")) {
        // Two consecutive calls — second must be >= first.
        struct timeval a, b;
        if (wali_gettimeofday(&a, NULL) != 0) return -1;
        if (wali_gettimeofday(&b, NULL) != 0) return -1;
        if (b.tv_sec < a.tv_sec) return -1;
        if (b.tv_sec == a.tv_sec && b.tv_usec < a.tv_usec) return -1;
        return 0;
    }
    return -1;
}
