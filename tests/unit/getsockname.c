// CMD: args="bound"
// CMD: args="unbound"
// CMD: args="bad_fd"

#include "wali_start.c"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getsockname")))
long __imported_wali_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int wali_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return (int)__imported_wali_getsockname(sockfd, addr, addrlen);
}
#else
#include <sys/syscall.h>
int wali_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return syscall(SYS_getsockname, sockfd, addr, addrlen);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "bound";

    if (!strcmp(mode, "bad_fd")) {
        struct sockaddr_in name;
        socklen_t nlen = sizeof(name);
        long r = wali_getsockname(99999, (struct sockaddr*)&name, &nlen);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "bound")) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (wali_syscall_bind(fd, &addr, sizeof(addr)) != 0) goto out;

        struct sockaddr_in name;
        socklen_t nlen = sizeof(name);
        memset(&name, 0, sizeof(name));
        long r = wali_getsockname(fd, (struct sockaddr*)&name, &nlen);
        if (r != 0) goto out;
        if (name.sin_family != AF_INET) goto out;
        if (name.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) goto out;
        if (name.sin_port == 0) goto out;  // kernel must have assigned an ephemeral port
        ret = 0;
    } else if (!strcmp(mode, "unbound")) {
        // Unbound socket: getsockname succeeds, returns AF_INET with all-zero addr/port.
        struct sockaddr_in name;
        socklen_t nlen = sizeof(name);
        memset(&name, 0xAA, sizeof(name));
        long r = wali_getsockname(fd, (struct sockaddr*)&name, &nlen);
        if (r != 0) goto out;
        if (name.sin_family != AF_INET) goto out;
        if (name.sin_addr.s_addr != 0) goto out;
        if (name.sin_port != 0) goto out;
        ret = 0;
    }

out:
    wali_syscall_close(fd);
    return ret;
}
