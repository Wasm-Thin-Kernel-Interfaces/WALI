// CMD: args="lower_soft"
// CMD: args="bad_resource"
// CMD: args="raise_above_hard"

#include "wali_start.c"
#include <sys/resource.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setrlimit")))
long __imported_wali_setrlimit(int resource, const struct rlimit *rlim);
__attribute__((__import_module__("wali"), __import_name__("SYS_getrlimit")))
long __imported_wali_getrlimit(int resource, struct rlimit *rlim);
int wali_setrlimit(int r, const struct rlimit *l) { return (int)__imported_wali_setrlimit(r, l); }
int wali_getrlimit(int r, struct rlimit *l) { return (int)__imported_wali_getrlimit(r, l); }
#else
#include <sys/syscall.h>
int wali_setrlimit(int r, const struct rlimit *l) { return syscall(SYS_setrlimit, r, l); }
int wali_getrlimit(int r, struct rlimit *l) { return syscall(SYS_getrlimit, r, l); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "lower_soft";

    if (!strcmp(mode, "bad_resource")) {
        struct rlimit lim = { 1024, 1024 };
        long r = wali_setrlimit(9999, &lim);
        return (r < 0) ? 0 : -1;
    }

    struct rlimit cur;
    if (wali_getrlimit(RLIMIT_NOFILE, &cur) != 0) return -1;

    if (!strcmp(mode, "lower_soft")) {
        if (cur.rlim_cur == 0) return 0;
        struct rlimit nl = cur;
        nl.rlim_cur -= 1;
        if (wali_setrlimit(RLIMIT_NOFILE, &nl) != 0) return -1;
        struct rlimit chk;
        if (wali_getrlimit(RLIMIT_NOFILE, &chk) != 0) return -1;
        return (chk.rlim_cur == nl.rlim_cur) ? 0 : -1;
    }
    if (!strcmp(mode, "raise_above_hard")) {
        // Setting soft above hard fails for non-root.
        struct rlimit nl = cur;
        if (nl.rlim_max < (rlim_t)-2) nl.rlim_cur = nl.rlim_max + 1;
        else return 0;
        long r = wali_setrlimit(RLIMIT_NOFILE, &nl);
        return (r == 0) ? 0 : 1;
    }
    return -1;
}
