// CMD: args="timeout_zero_no_event"
// CMD: args="event_ready"
// CMD: args="bad_maxevents"

#include "wali_start.c"
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

#ifdef __wasm__
WALI_IMPORT("SYS_epoll_create1") long wali_syscall_epoll_create1(int flags);
WALI_IMPORT("SYS_epoll_ctl") long wali_syscall_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
WALI_IMPORT("SYS_epoll_pwait") long wali_syscall_epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const void *sigmask, int sigsetsize);

int wali_epoll_create1(int flags) { return (int)wali_syscall_epoll_create1(flags); }
int wali_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) { return (int)wali_syscall_epoll_ctl(epfd, op, fd, event); }
int wali_epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    return (int)wali_syscall_epoll_pwait(epfd, events, maxevents, timeout, NULL, 8);
}
#else
#include <sys/syscall.h>
int wali_epoll_create1(int flags) { return syscall(SYS_epoll_create1, flags); }
int wali_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) { return syscall(SYS_epoll_ctl, epfd, op, fd, event); }
int wali_epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    return syscall(SYS_epoll_pwait, epfd, events, maxevents, timeout, NULL, 8);
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "timeout_zero_no_event";

    int epfd = wali_epoll_create1(0);
    TEST_ASSERT(epfd >= 0);
    int pfd[2];
    TEST_ASSERT_EQ(pipe(pfd), 0);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = pfd[0];
    TEST_ASSERT_EQ(wali_epoll_ctl(epfd, EPOLL_CTL_ADD, pfd[0], &ev), 0);

    struct epoll_event events[2];
    int n;
    int ret = -1;

    if (!strcmp(mode, "timeout_zero_no_event")) {
        n = wali_epoll_pwait(epfd, events, 2, 0);
        ret = (n == 0) ? 0 : -1;
    } else if (!strcmp(mode, "event_ready")) {
        TEST_ASSERT_EQ(write(pfd[1], "X", 1), 1);
        n = wali_epoll_pwait(epfd, events, 2, 1000);
        ret = (n == 1 && events[0].data.fd == pfd[0] && (events[0].events & EPOLLIN)) ? 0 : -1;
    } else if (!strcmp(mode, "bad_maxevents")) {
        n = wali_epoll_pwait(epfd, events, 0, 0);  // maxevents <= 0 → EINVAL
        ret = (n < 0) ? 0 : -1;
    }

    wali_syscall_close(pfd[0]); wali_syscall_close(pfd[1]); wali_syscall_close(epfd);
    return ret;
}
