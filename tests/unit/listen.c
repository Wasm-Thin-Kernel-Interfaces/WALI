// CMD: args="backlog_5"
// CMD: args="backlog_0"
// CMD: args="bad_fd"
// CMD: args="udp"

#include "wali_start.c"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_listen")))
long __imported_wali_listen(int sockfd, int backlog);
int wali_listen(int sockfd, int backlog) { return (int)__imported_wali_listen(sockfd, backlog); }
#else
#include <sys/syscall.h>
int wali_listen(int sockfd, int backlog) { return syscall(SYS_listen, sockfd, backlog); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "backlog_5";

    if (!strcmp(mode, "bad_fd")) {
        long r = wali_listen(99999, 5);
        return (r < 0) ? 0 : -1;
    }

    int type = !strcmp(mode, "udp") ? SOCK_DGRAM : SOCK_STREAM;
    int fd = wali_syscall_socket(AF_INET, type, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (wali_syscall_bind(fd, &addr, sizeof(addr)) != 0) { wali_syscall_close(fd); return -1; }

    int backlog = 5;
    int expect_ok = 1;
    if (!strcmp(mode, "backlog_0")) backlog = 0;
    else if (!strcmp(mode, "udp"))  expect_ok = 0;  // listen on UDP must fail.

    long r = wali_listen(fd, backlog);
    wali_syscall_close(fd);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
