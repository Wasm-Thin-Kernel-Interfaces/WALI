// CMD: args="self_zero"
// CMD: args="nonexistent_pid"
// CMD: args="bad_signal"

#include "wali_start.c"
#include <signal.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_kill")))
long __imported_wali_kill(int pid, int sig);
__attribute__((__import_module__("wali"), __import_name__("SYS_getpid")))
long __imported_wali_getpid(void);
int wali_kill(int pid, int sig) { return (int)__imported_wali_kill(pid, sig); }
int wali_getpid(void) { return (int)__imported_wali_getpid(); }
#else
#include <sys/syscall.h>
int wali_kill(int pid, int sig) { return syscall(SYS_kill, pid, sig); }
pid_t wali_getpid(void) { return syscall(SYS_getpid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self_zero";

    if (!strcmp(mode, "self_zero")) {
        // sig=0 is the existence check; should always succeed for self.
        return (wali_kill(wali_getpid(), 0) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "nonexistent_pid")) {
        long r = wali_kill(2147483640, 0);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_signal")) {
        long r = wali_kill(wali_getpid(), 999);
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
