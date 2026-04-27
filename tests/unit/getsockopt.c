// CMD: args="reuseaddr"
// CMD: args="so_type"
// CMD: args="bad_fd"
// CMD: args="bad_optname"

#include "wali_start.c"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setsockopt")))
long __imported_wali_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
__attribute__((__import_module__("wali"), __import_name__("SYS_getsockopt")))
long __imported_wali_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int wali_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) { return (int)__imported_wali_setsockopt(sockfd, level, optname, optval, optlen); }
int wali_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) { return (int)__imported_wali_getsockopt(sockfd, level, optname, optval, optlen); }
#else
#include <sys/syscall.h>
int wali_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) { return syscall(SYS_setsockopt, sockfd, level, optname, optval, optlen); }
int wali_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) { return syscall(SYS_getsockopt, sockfd, level, optname, optval, optlen); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "reuseaddr";

    if (!strcmp(mode, "bad_fd")) {
        int v;
        socklen_t len = sizeof(v);
        long r = wali_getsockopt(99999, SOL_SOCKET, SO_REUSEADDR, &v, &len);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "reuseaddr")) {
        int reuse = 1;
        if (wali_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) goto out;
        int v = 0;
        socklen_t len = sizeof(v);
        if (wali_getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, &len) != 0) goto out;
        ret = (v != 0) ? 0 : -1;
    } else if (!strcmp(mode, "so_type")) {
        int v = 0;
        socklen_t len = sizeof(v);
        if (wali_getsockopt(fd, SOL_SOCKET, SO_TYPE, &v, &len) != 0) goto out;
        ret = (v == SOCK_STREAM) ? 0 : -1;
    } else if (!strcmp(mode, "bad_optname")) {
        int v;
        socklen_t len = sizeof(v);
        long r = wali_getsockopt(fd, SOL_SOCKET, 99999, &v, &len);
        ret = (r < 0) ? 0 : -1;
    }

out:
    wali_syscall_close(fd);
    return ret;
}
