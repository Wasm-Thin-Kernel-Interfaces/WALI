// CMD: args="ok"
// CMD: args="empty_nonblock"
// CMD: args="bad_fd"

#include "wali_start.c"
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_socketpair")))
long __imported_wali_socketpair(int domain, int type, int protocol, int sv[2]);
__attribute__((__import_module__("wali"), __import_name__("SYS_sendmsg")))
long __imported_wali_sendmsg(int sockfd, const struct msghdr *msg, int flags);
__attribute__((__import_module__("wali"), __import_name__("SYS_recvmsg")))
long __imported_wali_recvmsg(int sockfd, struct msghdr *msg, int flags);
int wali_socketpair(int d, int t, int p, int sv[2]) { return (int)__imported_wali_socketpair(d, t, p, sv); }
ssize_t wali_sendmsg(int s, const struct msghdr *m, int f) { return (ssize_t)__imported_wali_sendmsg(s, m, f); }
ssize_t wali_recvmsg(int s, struct msghdr *m, int f) { return (ssize_t)__imported_wali_recvmsg(s, m, f); }
#else
#include <sys/syscall.h>
int wali_socketpair(int d, int t, int p, int sv[2]) { return syscall(SYS_socketpair, d, t, p, sv); }
ssize_t wali_sendmsg(int s, const struct msghdr *m, int f) { return syscall(SYS_sendmsg, s, m, f); }
ssize_t wali_recvmsg(int s, struct msghdr *m, int f) { return syscall(SYS_recvmsg, s, m, f); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    if (!strcmp(mode, "bad_fd")) {
        char buf[8];
        struct iovec iov = { buf, sizeof(buf) };
        struct msghdr m = {0};
        m.msg_iov = &iov; m.msg_iovlen = 1;
        long r = wali_recvmsg(99999, &m, 0);
        return (r < 0) ? 0 : -1;
    }

    int sv[2];
    if (wali_socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) return -1;
    int ret = -1;

    if (!strcmp(mode, "ok")) {
        char sd[] = "MSG456";
        struct iovec siov = { sd, 6 };
        struct msghdr sm = {0};
        sm.msg_iov = &siov; sm.msg_iovlen = 1;
        if (wali_sendmsg(sv[0], &sm, 0) != 6) goto out;

        char rd[16] = {0};
        struct iovec riov = { rd, sizeof(rd) };
        struct msghdr rm = {0};
        rm.msg_iov = &riov; rm.msg_iovlen = 1;
        ssize_t n = wali_recvmsg(sv[1], &rm, 0);
        ret = (n == 6 && memcmp(rd, "MSG456", 6) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "empty_nonblock")) {
        wali_syscall_fcntl(sv[1], F_SETFL, O_NONBLOCK);
        char rd[8];
        struct iovec riov = { rd, sizeof(rd) };
        struct msghdr rm = {0};
        rm.msg_iov = &riov; rm.msg_iovlen = 1;
        ssize_t n = wali_recvmsg(sv[1], &rm, 0);
        ret = (n < 0) ? 0 : -1;
    }

out:
    wali_syscall_close(sv[0]);
    wali_syscall_close(sv[1]);
    return ret;
}
