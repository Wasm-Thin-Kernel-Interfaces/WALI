// CMD: args="exit_42"
// CMD: args="exit_0"
// CMD: args="any_pid"
// CMD: args="no_child"
// CMD: args="wnohang_no_child"

#include "wali_start.c"
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_wait4")))
long __imported_wali_wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage);
__attribute__((__import_module__("wali"), __import_name__("SYS_fork")))
long __imported_wali_fork(void);
__attribute__((__import_module__("wali"), __import_name__("SYS_exit")))
long __imported_wali_exit(int status);
pid_t wali_wait4(pid_t p, int *s, int o, struct rusage *r) { return (pid_t)__imported_wali_wait4(p, s, o, r); }
pid_t wali_fork(void) { return (pid_t)__imported_wali_fork(); }
void wali_exit(int s) { __imported_wali_exit(s); }
#else
pid_t wali_wait4(pid_t p, int *s, int o, struct rusage *r) { return wali_syscall_wait4(p, s, o, r); }
pid_t wali_fork(void) { return wali_syscall_fork(); }
void wali_exit(int s) { wali_syscall_exit(s); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "exit_42";

    if (!strcmp(mode, "no_child")) {
        // No children → ECHILD.
        int status = 0;
        long r = wali_wait4(-1, &status, 0, 0);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "wnohang_no_child")) {
        // WNOHANG with no children → ECHILD (per POSIX).
        int status = 0;
        long r = wali_wait4(-1, &status, WNOHANG, 0);
        return (r < 0) ? 0 : -1;
    }

    int code = !strcmp(mode, "exit_0") ? 0 : 42;
    pid_t pid = wali_fork();
    if (pid < 0) return -1;
    if (pid == 0) wali_exit(code);

    int status = 0;
    pid_t target = !strcmp(mode, "any_pid") ? -1 : pid;
    pid_t r = wali_wait4(target, &status, 0, 0);
    if (r != pid) return -1;
    if (!WIFEXITED(status)) return -1;
    return (WEXITSTATUS(status) == code) ? 0 : -1;
}
