// CMD: args="shut_wr"
// CMD: args="shut_rd"
// CMD: args="shut_rdwr"
// CMD: args="bad_fd"
// CMD: args="bad_how"

#include "wali_start.c"
#include <sys/socket.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_socketpair")))
long __imported_wali_socketpair(int d, int t, int p, int sv[2]);
__attribute__((__import_module__("wali"), __import_name__("SYS_shutdown")))
long __imported_wali_shutdown(int sockfd, int how);
int wali_socketpair(int d, int t, int p, int sv[2]) { return (int)__imported_wali_socketpair(d, t, p, sv); }
int wali_shutdown(int s, int h) { return (int)__imported_wali_shutdown(s, h); }
#else
#include <sys/syscall.h>
int wali_socketpair(int d, int t, int p, int sv[2]) { return syscall(SYS_socketpair, d, t, p, sv); }
int wali_shutdown(int s, int h) { return syscall(SYS_shutdown, s, h); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "shut_wr";

    if (!strcmp(mode, "bad_fd")) {
        long r = wali_shutdown(99999, SHUT_RDWR);
        return (r < 0) ? 0 : -1;
    }

    int sv[2];
    if (wali_socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) return -1;

    int how;
    int expect_ok = 1;
    if (!strcmp(mode, "shut_wr"))         how = SHUT_WR;
    else if (!strcmp(mode, "shut_rd"))    how = SHUT_RD;
    else if (!strcmp(mode, "shut_rdwr"))  how = SHUT_RDWR;
    else if (!strcmp(mode, "bad_how"))    { how = 9999; expect_ok = 0; }
    else { wali_syscall_close(sv[0]); wali_syscall_close(sv[1]); return -1; }

    long r = wali_shutdown(sv[0], how);
    wali_syscall_close(sv[0]); wali_syscall_close(sv[1]);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
