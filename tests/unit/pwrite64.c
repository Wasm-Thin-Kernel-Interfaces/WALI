// CMD: setup="/tmp/pwrite_a"  args="middle       /tmp/pwrite_a"  cleanup="/tmp/pwrite_a"
// CMD: setup="/tmp/pwrite_b"  args="start        /tmp/pwrite_b"  cleanup="/tmp/pwrite_b"
// CMD: setup="/tmp/pwrite_c"  args="past_eof     /tmp/pwrite_c"  cleanup="/tmp/pwrite_c"
// CMD: setup="/tmp/pwrite_d"  args="negative_off /tmp/pwrite_d"  cleanup="/tmp/pwrite_d"
// CMD:                         args="bad_fd       /tmp/none"      cleanup=""

#define _GNU_SOURCE
#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_pwrite64")))
long long __imported_wali_pwrite64(int fd, const void *buf, size_t count, long long offset);
long wali_pwrite(int fd, const void *buf, size_t count, long offset) {
    return (long)__imported_wali_pwrite64(fd, buf, count, (long long)offset);
}
#else
#include <sys/syscall.h>
long wali_pwrite(int fd, const void *buf, size_t count, long offset) {
    return syscall(SYS_pwrite64, fd, buf, count, offset);
}
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
        long r = wali_pwrite(99999, "X", 1, 0);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_WRONLY, 0);
    if (fd < 0) return -1;

    long r;
    int ret = -1;
    if (!strcmp(mode, "middle")) {
        r = wali_pwrite(fd, "XY", 2, 2);
        ret = (r == 2) ? 0 : -1;
    } else if (!strcmp(mode, "start")) {
        r = wali_pwrite(fd, "abc", 3, 0);
        ret = (r == 3) ? 0 : -1;
    } else if (!strcmp(mode, "past_eof")) {
        // pwrite past EOF extends the file with a hole.
        r = wali_pwrite(fd, "Z", 1, 100);
        ret = (r == 1) ? 0 : -1;
    } else if (!strcmp(mode, "negative_off")) {
        r = wali_pwrite(fd, "X", 1, -1);
        ret = (r < 0) ? 0 : -1;
    }
    wali_syscall_close(fd);
    return ret;
}
