// CMD: args="cloexec"
// CMD: args="nonblock"
// CMD: args="both"
// CMD: args="none"

#include "wali_start.c"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "cloexec";

    int flags = 0;
    int expect_cloexec = 0, expect_nonblock = 0;
    if (strcmp(mode, "cloexec") == 0) {
        flags = SOCK_CLOEXEC; expect_cloexec = 1;
    } else if (strcmp(mode, "nonblock") == 0) {
        flags = SOCK_NONBLOCK; expect_nonblock = 1;
    } else if (strcmp(mode, "both") == 0) {
        flags = SOCK_CLOEXEC | SOCK_NONBLOCK; expect_cloexec = 1; expect_nonblock = 1;
    } else if (strcmp(mode, "none") == 0) {
        flags = 0;
    } else {
        return -1;
    }

    int lsock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(lsock >= 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    TEST_ASSERT_EQ(wali_syscall_bind(lsock, &addr, sizeof(addr)), 0);
    TEST_ASSERT_EQ(wali_syscall_listen(lsock, 1), 0);

    socklen_t len = sizeof(addr);
    TEST_ASSERT_EQ(wali_syscall_getsockname(lsock, &addr, &len), 0);

    int pid = wali_syscall_fork();
    TEST_ASSERT(pid >= 0);

    if (pid == 0) {
        wali_syscall_close(lsock);
        int csock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
        if (csock < 0) wali_syscall_exit(1);
        if (wali_syscall_connect(csock, &addr, sizeof(addr)) != 0) {
            wali_syscall_close(csock);
            wali_syscall_exit(1);
        }
        wali_syscall_close(csock);
        wali_syscall_exit(0);
    }

    struct sockaddr_in peer;
    socklen_t plen = sizeof(peer);
    int asock = wali_syscall_accept4(lsock, &peer, &plen, flags);

    int fd_flags = -1, fl_flags = -1;
    if (asock >= 0) {
        fd_flags = wali_syscall_fcntl(asock, F_GETFD, 0);
        fl_flags = wali_syscall_fcntl(asock, F_GETFL, 0);
    }

    int status;
    wali_syscall_wait4(pid, &status, 0, 0);

    wali_syscall_close(lsock);
    if (asock >= 0) wali_syscall_close(asock);

    TEST_ASSERT(asock >= 0);

    if (expect_cloexec) {
        TEST_ASSERT(fd_flags & FD_CLOEXEC);
    } else {
        TEST_ASSERT(!(fd_flags & FD_CLOEXEC));
    }
    if (expect_nonblock) {
        TEST_ASSERT(fl_flags & O_NONBLOCK);
    } else {
        TEST_ASSERT(!(fl_flags & O_NONBLOCK));
    }

    return 0;
}
