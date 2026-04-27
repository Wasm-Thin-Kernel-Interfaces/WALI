// CMD: args="ok"
// CMD: args="bad_fd"
// CMD: args="bad_dest"

#include "wali_start.c"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_sendto")))
long __imported_wali_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
__attribute__((__import_module__("wali"), __import_name__("SYS_recvfrom")))
long __imported_wali_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t wali_sendto(int s, const void *b, size_t l, int f, const struct sockaddr *d, socklen_t al) { return (ssize_t)__imported_wali_sendto(s, b, l, f, d, al); }
ssize_t wali_recvfrom(int s, void *b, size_t l, int f, struct sockaddr *sa, socklen_t *al) { return (ssize_t)__imported_wali_recvfrom(s, b, l, f, sa, al); }
#else
#include <sys/syscall.h>
ssize_t wali_sendto(int s, const void *b, size_t l, int f, const struct sockaddr *d, socklen_t al) { return syscall(SYS_sendto, s, b, l, f, d, al); }
ssize_t wali_recvfrom(int s, void *b, size_t l, int f, struct sockaddr *sa, socklen_t *al) { return syscall(SYS_recvfrom, s, b, l, f, sa, al); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    if (!strcmp(mode, "bad_fd")) {
        struct sockaddr_in d;
        memset(&d, 0, sizeof(d));
        d.sin_family = AF_INET;
        d.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        d.sin_port = htons(1);
        long r = wali_sendto(99999, "X", 1, 0, (struct sockaddr*)&d, sizeof(d));
        return (r < 0) ? 0 : -1;
    }

    int s1 = wali_syscall_socket(AF_INET, SOCK_DGRAM, 0);
    int s2 = wali_syscall_socket(AF_INET, SOCK_DGRAM, 0);
    if (s1 < 0 || s2 < 0) return -1;
    struct sockaddr_in a1, a2;
    memset(&a1, 0, sizeof(a1)); memset(&a2, 0, sizeof(a2));
    a1.sin_family = AF_INET; a1.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a1.sin_port = 0;
    a2 = a1;
    if (wali_syscall_bind(s1, &a1, sizeof(a1)) != 0) goto fail;
    if (wali_syscall_bind(s2, &a2, sizeof(a2)) != 0) goto fail;
    socklen_t alen = sizeof(a2);
    if (wali_syscall_getsockname(s2, &a2, &alen) != 0) goto fail;

    int ret = -1;
    if (!strcmp(mode, "ok")) {
        ret = (wali_sendto(s1, "HELLO", 5, 0, (struct sockaddr*)&a2, sizeof(a2)) == 5) ? 0 : -1;
    } else if (!strcmp(mode, "bad_dest")) {
        // Wrong family in dest sockaddr → fail.
        struct sockaddr_in bad = a2;
        bad.sin_family = AF_INET6;
        long r = wali_sendto(s1, "X", 1, 0, (struct sockaddr*)&bad, sizeof(bad));
        ret = (r < 0) ? 0 : -1;
    }

    wali_syscall_close(s1); wali_syscall_close(s2);
    return ret;
fail:
    wali_syscall_close(s1); wali_syscall_close(s2);
    return -1;
}
