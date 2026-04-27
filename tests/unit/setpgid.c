// CMD: args="self_same"
// CMD: args="bad_pid"
// CMD: args="negative_pgid"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setpgid")))
long __imported_wali_setpgid(pid_t pid, pid_t pgid);
__attribute__((__import_module__("wali"), __import_name__("SYS_getpgid")))
long __imported_wali_getpgid(pid_t pid);
int wali_setpgid(pid_t p, pid_t g) { return (int)__imported_wali_setpgid(p, g); }
int wali_getpgid(pid_t p) { return (int)__imported_wali_getpgid(p); }
#else
#include <sys/syscall.h>
int wali_setpgid(pid_t p, pid_t g) { return syscall(SYS_setpgid, p, g); }
int wali_getpgid(pid_t p) { return syscall(SYS_getpgid, p); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self_same";

    if (!strcmp(mode, "self_same")) {
        // Setting to current pgid is a no-op success.
        int p = wali_getpgid(0);
        if (p < 0) return -1;
        return (wali_setpgid(0, p) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_pid")) {
        // Nonexistent pid → ESRCH.
        long r = wali_setpgid(2147483640, 2147483640);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "negative_pgid")) {
        long r = wali_setpgid(0, -1);
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
