// CMD: args="ok"
// CMD: args="peeraddr"
// CMD: args="null_addr"
// CMD: args="not_listening"

#include "wali_start.c"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    int lsock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(lsock >= 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    TEST_ASSERT_EQ(wali_syscall_bind(lsock, &addr, sizeof(addr)), 0);

    if (strcmp(mode, "not_listening") == 0) {
        // accept on a bound-but-not-listening socket must fail (EINVAL).
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        long r = wali_syscall_accept(lsock, &peer, &plen);
        wali_syscall_close(lsock);
        TEST_ASSERT(r < 0);
        return 0;
    }

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

    long asock;
    struct sockaddr_in peer;
    socklen_t plen = sizeof(peer);
    memset(&peer, 0, sizeof(peer));

    if (strcmp(mode, "null_addr") == 0) {
        asock = wali_syscall_accept(lsock, NULL, NULL);
    } else {
        asock = wali_syscall_accept(lsock, &peer, &plen);
    }

    int status;
    wali_syscall_wait4(pid, &status, 0, 0);

    wali_syscall_close(lsock);
    if (asock >= 0) wali_syscall_close(asock);

    TEST_ASSERT(asock >= 0);

    if (strcmp(mode, "peeraddr") == 0) {
        TEST_ASSERT_EQ(peer.sin_family, AF_INET);
        TEST_ASSERT_NE(peer.sin_port, 0);
    }

    return 0;
}
