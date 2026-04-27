// CMD: args="add"
// CMD: args="mod"
// CMD: args="del"
// CMD: args="add_already"
// CMD: args="del_missing"
// CMD: args="bad_op"

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

int wali_epoll_create1(int flags) { return (int)wali_syscall_epoll_create1(flags); }
int wali_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) { return (int)wali_syscall_epoll_ctl(epfd, op, fd, event); }
#else
#include <sys/syscall.h>
int wali_epoll_create1(int flags) { return syscall(SYS_epoll_create1, flags); }
int wali_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) { return syscall(SYS_epoll_ctl, epfd, op, fd, event); }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "add";

    int epfd = wali_epoll_create1(0);
    TEST_ASSERT(epfd >= 0);
    int pfd[2];
    TEST_ASSERT_EQ(pipe(pfd), 0);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = pfd[0];

    long r;
    int expect_ok = 1;
    if (!strcmp(mode, "add")) {
        r = wali_epoll_ctl(epfd, EPOLL_CTL_ADD, pfd[0], &ev);
    } else if (!strcmp(mode, "mod")) {
        wali_epoll_ctl(epfd, EPOLL_CTL_ADD, pfd[0], &ev);  // setup
        ev.events = EPOLLOUT;
        r = wali_epoll_ctl(epfd, EPOLL_CTL_MOD, pfd[0], &ev);
    } else if (!strcmp(mode, "del")) {
        wali_epoll_ctl(epfd, EPOLL_CTL_ADD, pfd[0], &ev);
        r = wali_epoll_ctl(epfd, EPOLL_CTL_DEL, pfd[0], &ev);
    } else if (!strcmp(mode, "add_already")) {
        wali_epoll_ctl(epfd, EPOLL_CTL_ADD, pfd[0], &ev);
        r = wali_epoll_ctl(epfd, EPOLL_CTL_ADD, pfd[0], &ev);  // EEXIST
        expect_ok = 0;
    } else if (!strcmp(mode, "del_missing")) {
        r = wali_epoll_ctl(epfd, EPOLL_CTL_DEL, pfd[0], &ev);  // ENOENT
        expect_ok = 0;
    } else if (!strcmp(mode, "bad_op")) {
        r = wali_epoll_ctl(epfd, 0xDEAD, pfd[0], &ev);  // EINVAL
        expect_ok = 0;
    } else {
        wali_syscall_close(pfd[0]); wali_syscall_close(pfd[1]); wali_syscall_close(epfd);
        return -1;
    }

    int success = (r == 0);
    wali_syscall_close(pfd[0]); wali_syscall_close(pfd[1]); wali_syscall_close(epfd);
    return (success == expect_ok) ? 0 : -1;
}
