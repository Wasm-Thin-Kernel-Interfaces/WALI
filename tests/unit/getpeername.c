// CMD: args="ok"
// CMD: args="not_connected"
// CMD: args="bad_fd"

#include "wali_start.c"
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getpeername")))
long __imported_wali_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int wali_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen) { return (int)__imported_wali_getpeername(sockfd, addr, addrlen); }
#else
#include <sys/syscall.h>
int wali_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen) { return syscall(SYS_getpeername, sockfd, addr, addrlen); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    if (!strcmp(mode, "bad_fd")) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        long r = wali_getpeername(99999, (struct sockaddr*)&peer, &plen);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "not_connected")) {
        // Fresh, unconnected socket → ENOTCONN.
        int s = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) return -1;
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        long r = wali_getpeername(s, (struct sockaddr*)&peer, &plen);
        wali_syscall_close(s);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "ok")) {
        int lsock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
        if (lsock < 0) return -1;
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (wali_syscall_bind(lsock, &addr, sizeof(addr)) != 0) goto fail;
        if (wali_syscall_listen(lsock, 1) != 0) goto fail;
        socklen_t alen = sizeof(addr);
        if (wali_syscall_getsockname(lsock, &addr, &alen) != 0) goto fail;

        int pid = wali_syscall_fork();
        if (pid < 0) goto fail;
        if (pid == 0) {
            wali_syscall_close(lsock);
            int csock = wali_syscall_socket(AF_INET, SOCK_STREAM, 0);
            if (csock < 0) wali_syscall_exit(1);
            if (wali_syscall_connect(csock, &addr, sizeof(addr)) != 0) wali_syscall_exit(1);
            wali_syscall_close(csock);
            wali_syscall_exit(0);
        }
        struct sockaddr_in client;
        socklen_t clen = sizeof(client);
        int asock = wali_syscall_accept(lsock, &client, &clen);
        int status;
        wali_syscall_wait4(pid, &status, 0, 0);
        if (asock < 0) goto fail;

        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        long r = wali_getpeername(asock, (struct sockaddr*)&peer, &plen);
        int peer_ok = (r == 0) && (peer.sin_addr.s_addr == htonl(INADDR_LOOPBACK)) && (peer.sin_family == AF_INET);
        wali_syscall_close(asock);
        wali_syscall_close(lsock);
        return peer_ok ? 0 : -1;
fail:
        wali_syscall_close(lsock);
        return -1;
    }
    return -1;
}
