// CMD: args="ok"
// CMD: args="empty_iov"
// CMD: args="bad_fd"

#include "wali_start.c"
#include <sys/socket.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_socketpair")))
long __imported_wali_socketpair(int domain, int type, int protocol, int sv[2]);
__attribute__((__import_module__("wali"), __import_name__("SYS_sendmsg")))
long __imported_wali_sendmsg(int sockfd, const struct msghdr *msg, int flags);
int wali_socketpair(int d, int t, int p, int sv[2]) { return (int)__imported_wali_socketpair(d, t, p, sv); }
ssize_t wali_sendmsg(int s, const struct msghdr *m, int f) { return (ssize_t)__imported_wali_sendmsg(s, m, f); }
#else
#include <sys/syscall.h>
int wali_socketpair(int d, int t, int p, int sv[2]) { return syscall(SYS_socketpair, d, t, p, sv); }
ssize_t wali_sendmsg(int s, const struct msghdr *m, int f) { return syscall(SYS_sendmsg, s, m, f); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    if (!strcmp(mode, "bad_fd")) {
        char d[] = "X";
        struct iovec iov = { d, 1 };
        struct msghdr m = {0}; m.msg_iov = &iov; m.msg_iovlen = 1;
        long r = wali_sendmsg(99999, &m, 0);
        return (r < 0) ? 0 : -1;
    }

    int sv[2];
    if (wali_socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) return -1;
    int ret = -1;

    if (!strcmp(mode, "ok")) {
        char d[] = "TEST123";
        struct iovec iov = { d, 7 };
        struct msghdr m = {0}; m.msg_iov = &iov; m.msg_iovlen = 1;
        ret = (wali_sendmsg(sv[0], &m, 0) == 7) ? 0 : -1;
    } else if (!strcmp(mode, "empty_iov")) {
        struct msghdr m = {0};
        ret = (wali_sendmsg(sv[0], &m, 0) == 0) ? 0 : -1;
    }

    wali_syscall_close(sv[0]); wali_syscall_close(sv[1]);
    return ret;
}
