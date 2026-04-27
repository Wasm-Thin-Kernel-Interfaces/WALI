// CMD: args="ok"
// CMD: args="refused"
// CMD: args="wrong_family"

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

    if (!strcmp(mode, "ok")) {
        // Bring up a listener, capture port, connect to it.
        int lsock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
        TEST_ASSERT(lsock >= 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        TEST_ASSERT_EQ(wali_syscall_bind(lsock, &addr, sizeof(addr)), 0);
        TEST_ASSERT_EQ(wali_syscall_listen(lsock, 1), 0);
        socklen_t alen = sizeof(addr);
        TEST_ASSERT_EQ(wali_syscall_getsockname(lsock, &addr, &alen), 0);

        int csock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
        TEST_ASSERT(csock >= 0);
        long r = wali_syscall_connect(csock, &addr, sizeof(addr));
        wali_syscall_close(csock);
        wali_syscall_close(lsock);
        return (r == 0) ? 0 : -1;
    }

    if (!strcmp(mode, "refused")) {
        // No listener on port 1 → ECONNREFUSED.
        int csock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
        TEST_ASSERT(csock >= 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(1);
        long r = wali_syscall_connect(csock, &addr, sizeof(addr));
        wali_syscall_close(csock);
        return (r < 0) ? 0 : -1;
    }

    if (!strcmp(mode, "wrong_family")) {
        int csock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
        TEST_ASSERT(csock >= 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET6;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(1);
        long r = wali_syscall_connect(csock, &addr, sizeof(addr));
        wali_syscall_close(csock);
        return (r < 0) ? 0 : -1;
    }

    return -1;
}
