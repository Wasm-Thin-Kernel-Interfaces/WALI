// CMD: args="empty_timeout"
// CMD: args="ready_after_write"
// CMD: args="timeout_zero"
// CMD: args="nfds_zero"

#include "wali_start.c"
#include <poll.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_poll")))
long __imported_wali_poll(struct pollfd *fds, nfds_t nfds, int timeout);
int wali_poll(struct pollfd *fds, nfds_t nfds, int timeout) { return (int)__imported_wali_poll(fds, nfds, timeout); }
#else
#include <sys/syscall.h>
int wali_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
#ifdef SYS_poll
    return syscall(SYS_poll, fds, nfds, timeout);
#else
    struct timespec ts = {timeout / 1000, (timeout % 1000) * 1000000L};
    return syscall(SYS_ppoll, fds, nfds, timeout >= 0 ? &ts : NULL, NULL, 0);
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
        // poll(NULL, 0, 5) is a portable sleep-for-5ms with no fds.
        long r = wali_poll(NULL, 0, 5);
        return (r == 0) ? 0 : -1;
    }

    int pfd[2];
    if (wali_syscall_pipe2(pfd, 0) != 0) return -1;
    struct pollfd p = { .fd = pfd[0], .events = POLLIN, .revents = 0 };
    int ret = -1;

    if (!strcmp(mode, "empty_timeout")) {
        // Empty pipe + 10ms timeout → returns 0.
        long r = wali_poll(&p, 1, 10);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "timeout_zero")) {
        // Non-blocking poll on empty pipe → returns 0 immediately.
        long r = wali_poll(&p, 1, 0);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "ready_after_write")) {
        if (wali_syscall_write(pfd[1], "X", 1) != 1) goto out;
        long r = wali_poll(&p, 1, 100);
        ret = (r == 1 && (p.revents & POLLIN)) ? 0 : -1;
    }

out:
    wali_syscall_close(pfd[0]);
    wali_syscall_close(pfd[1]);
    return ret;
}
