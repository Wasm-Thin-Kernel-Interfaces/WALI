// CMD: args="self_zero"
// CMD: args="bad_tid"
// CMD: args="bad_signal"

#include "wali_start.c"
#include <signal.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_tkill")))
long __imported_wali_tkill(int tid, int sig);
__attribute__((__import_module__("wali"), __import_name__("SYS_gettid")))
long __imported_wali_gettid(void);
int wali_tkill(int t, int s) { return (int)__imported_wali_tkill(t, s); }
int wali_gettid(void) { return (int)__imported_wali_gettid(); }
#else
#include <sys/syscall.h>
int wali_tkill(int t, int s) { return syscall(SYS_tkill, t, s); }
int wali_gettid(void) { return syscall(SYS_gettid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self_zero";

    if (!strcmp(mode, "self_zero")) {
        return (wali_tkill(wali_gettid(), 0) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_tid")) {
        long r = wali_tkill(2147483640, 0);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_signal")) {
        long r = wali_tkill(wali_gettid(), 999);
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
