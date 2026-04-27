// CMD: setup="/tmp/pread_a"  args="middle       /tmp/pread_a"  cleanup="/tmp/pread_a"
// CMD: setup="/tmp/pread_b"  args="start        /tmp/pread_b"  cleanup="/tmp/pread_b"
// CMD: setup="/tmp/pread_c"  args="past_eof     /tmp/pread_c"  cleanup="/tmp/pread_c"
// CMD: setup="/tmp/pread_d"  args="negative_off /tmp/pread_d"  cleanup="/tmp/pread_d"
// CMD:                        args="bad_fd       /tmp/none"     cleanup=""

#define _GNU_SOURCE
#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_pread64")))
long long __imported_wali_pread64(int fd, void *buf, size_t count, long long offset);
long wali_pread(int fd, void *buf, size_t count, long offset) {
    return (long)__imported_wali_pread64(fd, buf, count, (long long)offset);
}
#else
#include <sys/syscall.h>
long wali_pread(int fd, void *buf, size_t count, long offset) {
    return syscall(SYS_pread64, fd, buf, count, offset);
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
        char buf[4];
        long r = wali_pread(99999, buf, 4, 0);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;

    char buf[16] = {0};
    long r;
    int ret = -1;

    if (!strcmp(mode, "middle")) {
        r = wali_pread(fd, buf, 3, 2);
        ret = (r == 3 && memcmp(buf, "234", 3) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "start")) {
        r = wali_pread(fd, buf, 10, 0);
        ret = (r == 10 && memcmp(buf, "0123456789", 10) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "past_eof")) {
        r = wali_pread(fd, buf, 4, 100);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "negative_off")) {
        r = wali_pread(fd, buf, 4, -1);
        ret = (r < 0) ? 0 : -1;
    }

    wali_syscall_close(fd);
    return ret;
}
