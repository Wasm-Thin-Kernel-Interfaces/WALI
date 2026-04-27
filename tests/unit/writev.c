// CMD: setup="/tmp/wv_a"  args="two     /tmp/wv_a"  cleanup="content:ABCD /tmp/wv_a"
// CMD: setup="/tmp/wv_b"  args="single  /tmp/wv_b"  cleanup="content:HELLO /tmp/wv_b"
// CMD: setup="/tmp/wv_c"  args="cnt_zero /tmp/wv_c" cleanup="/tmp/wv_c"
// CMD:                     args="bad_fd  /tmp/none"  cleanup=""

#define _GNU_SOURCE
#include "wali_start.c"
#include <fcntl.h>
#include <sys/uio.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_writev")))
long __imported_wali_writev(int fd, const struct iovec *iov, int iovcnt);
long wali_writev(int fd, const struct iovec *iov, int iovcnt) { return (long)__imported_wali_writev(fd, iov, iovcnt); }
#else
#include <sys/syscall.h>
long wali_writev(int fd, const struct iovec *iov, int iovcnt) { return syscall(SYS_writev, fd, iov, iovcnt); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc == 0) return 0;
    if (argc == 1) { unlink(argv[0]); return 0; }
    // Args: "content:<expected>" <path>
    const char *expected = NULL;
    const char *path = NULL;
    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "content:", 8)) expected = argv[i] + 8;
        else path = argv[i];
    }
    if (!path) return -1;
    int ret = 0;
    if (expected) {
        int fd = open(path, O_RDONLY);
        if (fd < 0) ret = -1;
        else {
            char buf[64] = {0};
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n != (ssize_t)strlen(expected) || memcmp(buf, expected, n) != 0) ret = -1;
        }
    }
    unlink(path);
    return ret;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode, "bad_fd")) {
        struct iovec iov = { "X", 1 };
        long r = wali_writev(99999, &iov, 1);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_WRONLY, 0);
    if (fd < 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "two")) {
        char *a = "AB", *b = "CD";
        struct iovec iov[2] = { {a, 2}, {b, 2} };
        long r = wali_writev(fd, iov, 2);
        ret = (r == 4) ? 0 : -1;
    } else if (!strcmp(mode, "single")) {
        char *a = "HELLO";
        struct iovec iov = { a, 5 };
        long r = wali_writev(fd, &iov, 1);
        ret = (r == 5) ? 0 : -1;
    } else if (!strcmp(mode, "cnt_zero")) {
        long r = wali_writev(fd, NULL, 0);
        ret = (r == 0) ? 0 : -1;
    }
    wali_syscall_close(fd);
    return ret;
}
