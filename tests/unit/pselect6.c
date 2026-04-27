// CMD: args="ready"
// CMD: args="timeout"
// CMD: args="nfds_zero"
// CMD: args="null_timeout_with_data"

#include "wali_start.c"
#include <sys/select.h>
#include <time.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_pselect6")))
long __imported_wali_pselect6(int nfds, fd_set *r, fd_set *w, fd_set *e, const struct timespec *t, const void *m);
int wali_pselect(int n, fd_set *r, fd_set *w, fd_set *e, const struct timespec *t, const void *m) {
    return (int)__imported_wali_pselect6(n, r, w, e, t, m);
}
#else
#include <sys/syscall.h>
int wali_pselect(int n, fd_set *r, fd_set *w, fd_set *e, const struct timespec *t, const void *m) {
    return syscall(SYS_pselect6, n, r, w, e, t, m);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ready";

    if (!strcmp(mode, "nfds_zero")) {
        // pselect(0, NULL, ..., timeout, NULL) acts as a portable sleep.
        struct timespec ts = {0, 1000000};  // 1ms
        long r = wali_pselect(0, NULL, NULL, NULL, &ts, NULL);
        return (r == 0) ? 0 : -1;
    }

    int pfd[2];
    if (wali_syscall_pipe2(pfd, 0) != 0) return -1;
    fd_set rfds;
    int ret = -1;

    if (!strcmp(mode, "ready")) {
        if (wali_syscall_write(pfd[1], "a", 1) != 1) goto out;
        FD_ZERO(&rfds); FD_SET(pfd[0], &rfds);
        struct timespec ts = {0, 1000000};
        long r = wali_pselect(pfd[0] + 1, &rfds, NULL, NULL, &ts, NULL);
        ret = (r == 1 && FD_ISSET(pfd[0], &rfds)) ? 0 : -1;
    } else if (!strcmp(mode, "timeout")) {
        FD_ZERO(&rfds); FD_SET(pfd[0], &rfds);
        struct timespec ts = {0, 1000000};
        long r = wali_pselect(pfd[0] + 1, &rfds, NULL, NULL, &ts, NULL);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "null_timeout_with_data")) {
        if (wali_syscall_write(pfd[1], "a", 1) != 1) goto out;
        FD_ZERO(&rfds); FD_SET(pfd[0], &rfds);
        long r = wali_pselect(pfd[0] + 1, &rfds, NULL, NULL, NULL, NULL);
        ret = (r == 1) ? 0 : -1;
    }

out:
    wali_syscall_close(pfd[0]);
    wali_syscall_close(pfd[1]);
    return ret;
}
