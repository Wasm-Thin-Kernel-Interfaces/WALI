// CMD: args="basic"
// CMD: args="empty_read_after_close"
// CMD: args="bad_arg"

#include "wali_start.c"
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_pipe")))
long __imported_wali_pipe(int *pipefd);
int wali_pipe(int *pipefd) { return (int)__imported_wali_pipe(pipefd); }
#else
#include <sys/syscall.h>
int wali_pipe(int *pipefd) {
#ifdef SYS_pipe
    return syscall(SYS_pipe, pipefd);
#else
    return syscall(SYS_pipe2, pipefd, 0);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    if (!strcmp(mode, "bad_arg")) {
        // pipe(NULL) → EFAULT. Some kernels may segfault before; treat any
        // non-zero result as expected.
        long r = wali_pipe(NULL);
        return (r != 0) ? 0 : -1;
    }

    int pipefd[2];
    if (wali_pipe(pipefd) != 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "basic")) {
        const char *msg = "PIPE";
        if (wali_syscall_write(pipefd[1], msg, 4) != 4) goto out;
        char buf[8] = {0};
        if (wali_syscall_read(pipefd[0], buf, 4) != 4) goto out;
        ret = (memcmp(buf, msg, 4) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "empty_read_after_close")) {
        // Close write end → read returns 0 (EOF).
        wali_syscall_close(pipefd[1]);
        char buf[1];
        long r = wali_syscall_read(pipefd[0], buf, 1);
        ret = (r == 0) ? 0 : -1;
        wali_syscall_close(pipefd[0]);
        return ret;
    }

out:
    wali_syscall_close(pipefd[0]);
    wali_syscall_close(pipefd[1]);
    return ret;
}
