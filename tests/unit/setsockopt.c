// CMD: args="reuseaddr"
// CMD: args="recv_buf"
// CMD: args="bad_fd"
// CMD: args="bad_optname"

#include "wali_start.c"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setsockopt")))
long __imported_wali_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int wali_setsockopt(int s, int l, int n, const void *v, socklen_t z) { return (int)__imported_wali_setsockopt(s, l, n, v, z); }
#else
#include <sys/syscall.h>
int wali_setsockopt(int s, int l, int n, const void *v, socklen_t z) { return syscall(SYS_setsockopt, s, l, n, v, z); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "reuseaddr";

    if (!strcmp(mode, "bad_fd")) {
        int v = 1;
        long r = wali_setsockopt(99999, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int ret = -1;

    if (!strcmp(mode, "reuseaddr")) {
        int v = 1;
        ret = (wali_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "recv_buf")) {
        int v = 4096;
        ret = (wali_setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &v, sizeof(v)) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "bad_optname")) {
        int v = 1;
        long r = wali_setsockopt(fd, SOL_SOCKET, 99999, &v, sizeof(v));
        ret = (r < 0) ? 0 : -1;
    }
    wali_syscall_close(fd);
    return ret;
}
