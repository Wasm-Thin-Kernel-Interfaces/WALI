// CMD: args="0"
// CMD: args="7"
// CMD: args="42"
// CMD: args="127"
// CMD: args="255"

#include "wali_start.c"
#include <sys/wait.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_exit")))
long __imported_wali_exit(int status);
__attribute__((__import_module__("wali"), __import_name__("SYS_fork")))
long __imported_wali_fork(void);
__attribute__((__import_module__("wali"), __import_name__("SYS_wait4")))
long __imported_wali_wait4(int pid, int *wstatus, int options, void *rusage);

void wali_exit(int status) { __imported_wali_exit(status); }
int wali_fork(void) { return (int)__imported_wali_fork(); }
int wali_wait4(int pid, int *wstatus, int options, void *rusage) {
    return (int)__imported_wali_wait4(pid, wstatus, options, rusage);
}
#else
void wali_exit(int status) { wali_syscall_exit(status); }
int wali_fork(void) { return wali_syscall_fork(); }
int wali_wait4(int pid, int *wstatus, int options, void *rusage) { return wali_syscall_wait4(pid, wstatus, options, rusage); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

static int parse_int(const char *s) {
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 2) return -1;
    int code = parse_int(argv[1]);

    int pid = wali_fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        wali_exit(code);
    }

    int status = 0;
    int ret = wali_wait4(pid, &status, 0, 0);
    if (ret != pid) return -1;
    if (!WIFEXITED(status)) return -1;
    return (WEXITSTATUS(status) == code) ? 0 : -1;
}
