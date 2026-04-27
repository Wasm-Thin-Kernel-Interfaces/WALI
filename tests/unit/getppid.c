// CMD: args="positive"
// CMD: args="idempotent"
// CMD: args="differs_from_self"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getppid")))
long __imported_wali_getppid(void);
__attribute__((__import_module__("wali"), __import_name__("SYS_getpid")))
long __imported_wali_getpid(void);
pid_t wali_getppid(void) { return (pid_t)__imported_wali_getppid(); }
pid_t wali_getpid(void) { return (pid_t)__imported_wali_getpid(); }
#else
#include <sys/syscall.h>
pid_t wali_getppid(void) { return syscall(SYS_getppid); }
pid_t wali_getpid(void) { return syscall(SYS_getpid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "positive";

    if (!strcmp(mode, "positive"))   return (wali_getppid() > 0) ? 0 : -1;
    if (!strcmp(mode, "idempotent")) {
        pid_t a = wali_getppid();
        pid_t b = wali_getppid();
        return (a == b) ? 0 : -1;
    }
    if (!strcmp(mode, "differs_from_self")) {
        // Parent pid must differ from our own pid.
        return (wali_getppid() != wali_getpid()) ? 0 : -1;
    }
    return -1;
}
