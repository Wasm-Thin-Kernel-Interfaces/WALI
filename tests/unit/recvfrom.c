// CMD: args="ok"
// CMD: args="empty_nonblock"
// CMD: args="bad_fd"
// CMD: args="peek"

#include "wali_start.c"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_sendto")))
long __imported_wali_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
__attribute__((__import_module__("wali"), __import_name__("SYS_recvfrom")))
long __imported_wali_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t wali_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *da, socklen_t al) { return (ssize_t)__imported_wali_sendto(sockfd, buf, len, flags, da, al); }
ssize_t wali_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *sa, socklen_t *al) { return (ssize_t)__imported_wali_recvfrom(sockfd, buf, len, flags, sa, al); }
#else
#include <sys/syscall.h>
ssize_t wali_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *da, socklen_t al) { return syscall(SYS_sendto, sockfd, buf, len, flags, da, al); }
ssize_t wali_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *sa, socklen_t *al) { return syscall(SYS_recvfrom, sockfd, buf, len, flags, sa, al); }
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
        long r = wali_recvfrom(99999, buf, 8, 0, NULL, NULL);
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
        if (wali_sendto(s1, "WORLD", 5, 0, (struct sockaddr*)&a2, sizeof(a2)) != 5) goto fail;
        char buf[16];
        struct sockaddr_in from;
        socklen_t fl = sizeof(from);
        ssize_t n = wali_recvfrom(s2, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fl);
        ret = (n == 5 && memcmp(buf, "WORLD", 5) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "empty_nonblock")) {
        wali_syscall_fcntl(s2, F_SETFL, O_NONBLOCK);
        char buf[8];
        ssize_t n = wali_recvfrom(s2, buf, 8, 0, NULL, NULL);
        ret = (n < 0) ? 0 : -1;
    } else if (!strcmp(mode, "peek")) {
        if (wali_sendto(s1, "PEEK", 4, 0, (struct sockaddr*)&a2, sizeof(a2)) != 4) goto fail;
        char buf1[8] = {0}, buf2[8] = {0};
        ssize_t n1 = wali_recvfrom(s2, buf1, 8, MSG_PEEK, NULL, NULL);
        ssize_t n2 = wali_recvfrom(s2, buf2, 8, 0, NULL, NULL);
        ret = (n1 == 4 && n2 == 4 && memcmp(buf1, "PEEK", 4) == 0 && memcmp(buf2, "PEEK", 4) == 0) ? 0 : -1;
    }

    wali_syscall_close(s1); wali_syscall_close(s2);
    return ret;
fail:
    wali_syscall_close(s1); wali_syscall_close(s2);
    return -1;
}
