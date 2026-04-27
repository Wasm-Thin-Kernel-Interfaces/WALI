// CMD: args="basic"
// CMD: args="exit_zero"
// CMD: args="exit_nonzero"
// CMD: args="wait_no_child"

#include "wali_start.c"
#include <sys/wait.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_fork")))
long __imported_wali_fork(void);
__attribute__((__import_module__("wali"), __import_name__("SYS_wait4")))
long __imported_wali_wait4(int pid, int *wstatus, int options, void *rusage);
__attribute__((__import_module__("wali"), __import_name__("SYS_exit")))
long __imported_wali_exit(int status);

int wali_fork(void) { return (int)__imported_wali_fork(); }
int wali_wait4(int pid, int *wstatus, int options, void *rusage) { return (int)__imported_wali_wait4(pid, wstatus, options, rusage); }
void wali_exit(int status) { __imported_wali_exit(status); while(1); }
#else
int wali_fork(void) { return wali_syscall_fork(); }
int wali_wait4(int pid, int *wstatus, int options, void *rusage) { return wali_syscall_wait4(pid, wstatus, options, rusage); }
void wali_exit(int status) { wali_syscall_exit(status); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    if (!strcmp(mode, "wait_no_child")) {
        // No children → wait4 must fail with ECHILD.
        int status = 0;
        int r = wali_wait4(-1, &status, 0, 0);
        return (r < 0) ? 0 : -1;
    }

    int expected_code = 0;
    if (!strcmp(mode, "basic"))            expected_code = 42;
    else if (!strcmp(mode, "exit_zero"))   expected_code = 0;
    else if (!strcmp(mode, "exit_nonzero")) expected_code = 99;
    else return -1;

    int pid = wali_fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        wali_exit(expected_code);
    }

    int status = 0;
    int res = wali_wait4(pid, &status, 0, 0);
    if (res != pid) return -1;
    if (!WIFEXITED(status)) return -1;
    return (WEXITSTATUS(status) == expected_code) ? 0 : -1;
}
