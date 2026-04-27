// CMD: args="positive"
// CMD: args="idempotent"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_gettid")))
long __imported_wali_gettid(void);
pid_t wali_gettid(void) { return (pid_t)__imported_wali_gettid(); }
#else
#include <sys/syscall.h>
pid_t wali_gettid(void) { return syscall(SYS_gettid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "positive";

    if (!strcmp(mode, "positive"))   return (wali_gettid() > 0) ? 0 : -1;
    if (!strcmp(mode, "idempotent")) {
        pid_t a = wali_gettid();
        pid_t b = wali_gettid();
        return (a == b && a > 0) ? 0 : -1;
    }
    return -1;
}
