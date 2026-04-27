// CMD: args="self_zero"
// CMD: args="self_explicit"
// CMD: args="bad_pid"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getpgid")))
long __imported_wali_getpgid(pid_t pid);
__attribute__((__import_module__("wali"), __import_name__("SYS_getpid")))
long __imported_wali_getpid(void);

int wali_getpgid(pid_t pid) { return (int)__imported_wali_getpgid(pid); }
int wali_getpid(void) { return (int)__imported_wali_getpid(); }
#else
#include <sys/syscall.h>
int wali_getpgid(pid_t pid) { return syscall(SYS_getpgid, pid); }
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
        // pid=0 means current process.
        int pgid = wali_getpgid(0);
        return (pgid >= 0) ? 0 : -1;
    }
    if (!strcmp(mode, "self_explicit")) {
        // getpgid(getpid()) must equal getpgid(0).
        int a = wali_getpgid(0);
        int b = wali_getpgid(wali_getpid());
        return (a >= 0 && a == b) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_pid")) {
        // Very large pid that surely doesn't exist → ESRCH.
        int r = wali_getpgid(2147483640);
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
