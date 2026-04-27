// CMD: args="empty_timeout"
// CMD: args="ready_after_write"
// CMD: args="timeout_zero"
// CMD: args="nfds_zero"

#include "wali_start.c"
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_select")))
long __imported_wali_select(int nfds, fd_set *r, fd_set *w, fd_set *e, struct timeval *t);
int wali_select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t) { return (int)__imported_wali_select(n, r, w, e, t); }
#else
#include <sys/syscall.h>
int wali_select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t) {
#ifdef SYS_select
    return syscall(SYS_select, n, r, w, e, t);
#else
    sigset_t s; sigemptyset(&s);
    struct timespec ts = { .tv_sec = t ? t->tv_sec : 0, .tv_nsec = t ? t->tv_usec * 1000L : 0 };
    return syscall(SYS_pselect6, n, r, w, e, t ? &ts : NULL, &s);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "empty_timeout";

    if (!strcmp(mode, "nfds_zero")) {
        struct timeval tv = {0, 1000};  // 1ms
        long r = wali_select(0, NULL, NULL, NULL, &tv);
        return (r == 0) ? 0 : -1;
    }

    int pfd[2];
    if (wali_syscall_pipe2(pfd, 0) != 0) return -1;
    fd_set rfds;
    int ret = -1;

    if (!strcmp(mode, "empty_timeout")) {
        FD_ZERO(&rfds); FD_SET(pfd[0], &rfds);
        struct timeval tv = {0, 1000};
        long r = wali_select(pfd[0] + 1, &rfds, NULL, NULL, &tv);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "timeout_zero")) {
        FD_ZERO(&rfds); FD_SET(pfd[0], &rfds);
        struct timeval tv = {0, 0};
        long r = wali_select(pfd[0] + 1, &rfds, NULL, NULL, &tv);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "ready_after_write")) {
        if (wali_syscall_write(pfd[1], "A", 1) != 1) goto out;
        FD_ZERO(&rfds); FD_SET(pfd[0], &rfds);
        struct timeval tv = {1, 0};
        long r = wali_select(pfd[0] + 1, &rfds, NULL, NULL, &tv);
        ret = (r == 1 && FD_ISSET(pfd[0], &rfds)) ? 0 : -1;
    }

out:
    wali_syscall_close(pfd[0]);
    wali_syscall_close(pfd[1]);
    return ret;
}
