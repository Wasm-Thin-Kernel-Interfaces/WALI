// CMD: args="loopback"
// CMD: args="any"
// CMD: args="wrong_family"
// CMD: args="unix_abstract"
// CMD: setup="clean" args="unix_path" cleanup="clean"

#include "wali_start.c"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stddef.h>

#define UNIX_PATH "/tmp/wali_bind_unix.sock"

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc >= 1 && strcmp(argv[0], "clean") == 0) {
        unlink(UNIX_PATH);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc >= 1 && strcmp(argv[0], "clean") == 0) {
        unlink(UNIX_PATH);
    }
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "loopback";

    int domain = (strncmp(mode, "unix", 4) == 0) ? AF_UNIX : AF_INET;
    int fd = wali_syscall_socket(domain, SOCK_DGRAM, 0);
    TEST_ASSERT(fd >= 0);

    long ret;
    int expect_ok = 1;

    if (strcmp(mode, "loopback") == 0 || strcmp(mode, "any") == 0) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(strcmp(mode, "loopback") == 0 ? INADDR_LOOPBACK : INADDR_ANY);
        addr.sin_port = 0;
        ret = wali_syscall_bind(fd, &addr, sizeof(addr));
    } else if (strcmp(mode, "wrong_family") == 0) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET6;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        ret = wali_syscall_bind(fd, &addr, sizeof(addr));
        expect_ok = 0;
    } else if (strcmp(mode, "unix_abstract") == 0) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        const char *name = "wali_bind_test_abstract";
        size_t nlen = strlen(name);
        addr.sun_path[0] = '\0';
        memcpy(&addr.sun_path[1], name, nlen);
        socklen_t alen = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + nlen);
        ret = wali_syscall_bind(fd, &addr, alen);
    } else if (strcmp(mode, "unix_path") == 0) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, UNIX_PATH, sizeof(addr.sun_path) - 1);
        ret = wali_syscall_bind(fd, &addr, sizeof(addr));
    } else {
        wali_syscall_close(fd);
        return -1;
    }

    wali_syscall_close(fd);
    int success = (ret == 0);
    return (success == expect_ok) ? 0 : -1;
}
