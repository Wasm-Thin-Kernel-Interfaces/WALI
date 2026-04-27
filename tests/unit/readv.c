// CMD: setup="/tmp/readv_a"  args="two     /tmp/readv_a"  cleanup="/tmp/readv_a"
// CMD: setup="/tmp/readv_b"  args="single  /tmp/readv_b"  cleanup="/tmp/readv_b"
// CMD: setup="/tmp/readv_c"  args="cnt_zero /tmp/readv_c" cleanup="/tmp/readv_c"
// CMD:                        args="bad_fd  /tmp/none"     cleanup=""

#define _GNU_SOURCE
#include "wali_start.c"
#include <fcntl.h>
#include <sys/uio.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_readv")))
long __imported_wali_readv(int fd, const struct iovec *iov, int iovcnt);
long wali_readv(int fd, const struct iovec *iov, int iovcnt) { return (long)__imported_wali_readv(fd, iov, iovcnt); }
#else
#include <sys/syscall.h>
long wali_readv(int fd, const struct iovec *iov, int iovcnt) { return syscall(SYS_readv, fd, iov, iovcnt); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    write(fd, "0123456789", 10);
    close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    unlink(argv[0]);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode, "bad_fd")) {
        struct iovec iov = { .iov_base = (char[8]){0}, .iov_len = 8 };
        long r = wali_readv(99999, &iov, 1);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "two")) {
        char b1[3], b2[3];
        struct iovec iov[2] = { {b1, 3}, {b2, 3} };
        long r = wali_readv(fd, iov, 2);
        ret = (r == 6 && memcmp(b1, "012", 3) == 0 && memcmp(b2, "345", 3) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "single")) {
        char b[10];
        struct iovec iov = { b, 10 };
        long r = wali_readv(fd, &iov, 1);
        ret = (r == 10 && memcmp(b, "0123456789", 10) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "cnt_zero")) {
        long r = wali_readv(fd, NULL, 0);
        ret = (r == 0) ? 0 : -1;
    }
    wali_syscall_close(fd);
    return ret;
}
