// CMD: args="positive"
// CMD: args="idempotent"

#include "wali_start.c"
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getpid")))
long __imported_wali_getpid(void);
int wali_getpid(void) { return (int)__imported_wali_getpid(); }
#else
#include <sys/syscall.h>
int wali_getpid(void) { return syscall(SYS_getpid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "positive";

    if (!strcmp(mode, "positive")) {
        return (wali_getpid() > 0) ? 0 : -1;
    }
    if (!strcmp(mode, "idempotent")) {
        int a = wali_getpid();
        int b = wali_getpid();
        return (a == b && a > 0) ? 0 : -1;
    }
    return -1;
}
