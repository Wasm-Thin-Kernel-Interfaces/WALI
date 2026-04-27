// CMD: args="both"
// CMD: args="cloexec_only"
// CMD: args="nonblock_only"
// CMD: args="none"
// CMD: args="bad_flags"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_pipe2")))
long __imported_wali_pipe2(int *pipefd, int flags);
int wali_pipe2(int *pipefd, int flags) { return (int)__imported_wali_pipe2(pipefd, flags); }
#else
#include <sys/syscall.h>
int wali_pipe2(int *pipefd, int flags) { return syscall(SYS_pipe2, pipefd, flags); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "both";

    int flags = 0;
    int expect_cloexec = 0, expect_nonblock = 0;
    int expect_ok = 1;
    if (!strcmp(mode, "both"))           { flags = O_CLOEXEC | O_NONBLOCK; expect_cloexec = 1; expect_nonblock = 1; }
    else if (!strcmp(mode, "cloexec_only"))  { flags = O_CLOEXEC; expect_cloexec = 1; }
    else if (!strcmp(mode, "nonblock_only")) { flags = O_NONBLOCK; expect_nonblock = 1; }
    else if (!strcmp(mode, "none"))      { flags = 0; }
    else if (!strcmp(mode, "bad_flags")) { flags = 0xDEAD0000; expect_ok = 0; }
    else return -1;

    int pipefd[2];
    long r = wali_pipe2(pipefd, flags);
    int success = (r == 0);
    if (success != expect_ok) return -1;
    if (!success) return 0;

    int ret = 0;
    long fd_flags = wali_syscall_fcntl(pipefd[0], F_GETFD, 0);
    long fl_flags = wali_syscall_fcntl(pipefd[0], F_GETFL, 0);
    if (fd_flags < 0 || fl_flags < 0) ret = -1;
    else {
        int has_cloexec = (fd_flags & FD_CLOEXEC) ? 1 : 0;
        int has_nonblock = (fl_flags & O_NONBLOCK) ? 1 : 0;
        if (has_cloexec != expect_cloexec || has_nonblock != expect_nonblock) ret = -1;
    }

    wali_syscall_close(pipefd[0]);
    wali_syscall_close(pipefd[1]);
    return ret;
}
