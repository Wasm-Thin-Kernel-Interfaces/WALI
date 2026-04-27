// CMD: args="nofile"
// CMD: args="stack"
// CMD: args="cpu"
// CMD: args="nproc"
// CMD: args="bad_resource"

#include "wali_start.c"
#include <sys/resource.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getrlimit")))
long __imported_wali_getrlimit(int resource, struct rlimit *rlim);
int wali_getrlimit(int resource, struct rlimit *rlim) { return (int)__imported_wali_getrlimit(resource, rlim); }
#else
#include <sys/syscall.h>
int wali_getrlimit(int resource, struct rlimit *rlim) { return syscall(SYS_getrlimit, resource, rlim); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "nofile";

    int res;
    int expect_ok = 1;
    if (!strcmp(mode, "nofile"))             res = RLIMIT_NOFILE;
    else if (!strcmp(mode, "stack"))         res = RLIMIT_STACK;
    else if (!strcmp(mode, "cpu"))           res = RLIMIT_CPU;
    else if (!strcmp(mode, "nproc"))         res = RLIMIT_NPROC;
    else if (!strcmp(mode, "bad_resource")) { res = 9999; expect_ok = 0; }
    else return -1;

    struct rlimit rlim;
    long r = wali_getrlimit(res, &rlim);
    int success = (r == 0);
    if (success != expect_ok) return -1;
    if (success) {
        if (rlim.rlim_cur > rlim.rlim_max) return -1;
    }
    return 0;
}
