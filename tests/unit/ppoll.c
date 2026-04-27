// CMD: args="empty_timeout"
// CMD: args="ready_after_write"
// CMD: args="null_timeout_with_data"
// CMD: args="nfds_zero"

#include "wali_start.c"
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_ppoll")))
long __imported_wali_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo, const sigset_t *mask, size_t sigsetsize);
int wali_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo, const sigset_t *mask, size_t sz) {
    return (int)__imported_wali_ppoll(fds, nfds, tmo, mask, sz);
}
#else
#include <sys/syscall.h>
int wali_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo, const sigset_t *mask, size_t sz) {
    return syscall(SYS_ppoll, fds, nfds, tmo, mask, sz);
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
        struct timespec ts = {0, 5000000};  // 5ms
        long r = wali_ppoll(NULL, 0, &ts, NULL, 0);
        return (r == 0) ? 0 : -1;
    }

    int pfd[2];
    if (wali_syscall_pipe2(pfd, 0) != 0) return -1;
    struct pollfd p = { .fd = pfd[0], .events = POLLIN, .revents = 0 };
    int ret = -1;

    if (!strcmp(mode, "empty_timeout")) {
        struct timespec ts = {0, 1000000};  // 1ms
        long r = wali_ppoll(&p, 1, &ts, NULL, 0);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "ready_after_write")) {
        if (wali_syscall_write(pfd[1], "X", 1) != 1) goto out;
        struct timespec ts = {1, 0};
        long r = wali_ppoll(&p, 1, &ts, NULL, 0);
        ret = (r == 1 && (p.revents & POLLIN)) ? 0 : -1;
    } else if (!strcmp(mode, "null_timeout_with_data")) {
        // NULL timeout = block forever, but we have data ready before calling.
        if (wali_syscall_write(pfd[1], "X", 1) != 1) goto out;
        long r = wali_ppoll(&p, 1, NULL, NULL, 0);
        ret = (r == 1 && (p.revents & POLLIN)) ? 0 : -1;
    }

out:
    wali_syscall_close(pfd[0]);
    wali_syscall_close(pfd[1]);
    return ret;
}
