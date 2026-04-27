// CMD: args="self_zero"
// CMD: args="self_explicit"
// CMD: args="bad_pid"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getsid")))
long __imported_wali_getsid(pid_t pid);
__attribute__((__import_module__("wali"), __import_name__("SYS_getpid")))
long __imported_wali_getpid(void);
int wali_getsid(pid_t pid) { return (int)__imported_wali_getsid(pid); }
int wali_getpid(void) { return (int)__imported_wali_getpid(); }
#else
#include <sys/syscall.h>
int wali_getsid(pid_t pid) { return syscall(SYS_getsid, pid); }
int wali_getpid(void) { return syscall(SYS_getpid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self_zero";

    if (!strcmp(mode, "self_zero")) {
        return (wali_getsid(0) >= 0) ? 0 : -1;
    }
    if (!strcmp(mode, "self_explicit")) {
        int a = wali_getsid(0);
        int b = wali_getsid(wali_getpid());
        return (a >= 0 && a == b) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_pid")) {
        int r = wali_getsid(2147483640);
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
