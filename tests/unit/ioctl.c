// CMD: args="fionbio"
// CMD: args="fionread"
// CMD: args="bad_fd"
// CMD: args="bad_request"

#include "wali_start.c"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_ioctl")))
long __imported_wali_ioctl(int fd, int request, void *arg);
__attribute__((__import_module__("wali"), __import_name__("SYS_socketpair")))
long __imported_wali_socketpair(int domain, int type, int protocol, int sv[2]);
int wali_ioctl(int fd, unsigned long req, void *arg) { return (int)__imported_wali_ioctl(fd, req, arg); }
int wali_socketpair(int d, int t, int p, int sv[2]) { return (int)__imported_wali_socketpair(d, t, p, sv); }
#else
#include <sys/syscall.h>
int wali_ioctl(int fd, unsigned long req, void *arg) { return syscall(SYS_ioctl, fd, req, arg); }
int wali_socketpair(int d, int t, int p, int sv[2]) { return syscall(SYS_socketpair, d, t, p, sv); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "fionbio";

    if (!strcmp(mode, "bad_fd")) {
        int on = 1;
        long r = wali_ioctl(99999, FIONBIO, &on);
        return (r < 0) ? 0 : -1;
    }

    int sv[2];
    if (wali_socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "fionbio")) {
        // Set non-blocking via ioctl, verify via fcntl(F_GETFL).
        int on = 1;
        if (wali_ioctl(sv[0], FIONBIO, &on) != 0) goto out;
        long fl = wali_syscall_fcntl(sv[0], F_GETFL, 0);
        ret = (fl >= 0 && (fl & O_NONBLOCK)) ? 0 : -1;
    } else if (!strcmp(mode, "fionread")) {
        // Write 5 bytes, ioctl FIONREAD must report 5.
        if (wali_syscall_write(sv[1], "hello", 5) != 5) goto out;
        int avail = 0;
        if (wali_ioctl(sv[0], FIONREAD, &avail) != 0) goto out;
        ret = (avail == 5) ? 0 : -1;
    } else if (!strcmp(mode, "bad_request")) {
        long r = wali_ioctl(sv[0], 0xDEADBEEF, NULL);
        ret = (r < 0) ? 0 : -1;
    }

out:
    wali_syscall_close(sv[0]);
    wali_syscall_close(sv[1]);
    return ret;
}
