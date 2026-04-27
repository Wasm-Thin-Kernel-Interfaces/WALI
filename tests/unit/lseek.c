// CMD: setup="/tmp/lseek_a"  args="set 5 5  /tmp/lseek_a"  cleanup="/tmp/lseek_a"
// CMD: setup="/tmp/lseek_b"  args="end 0 10 /tmp/lseek_b"  cleanup="/tmp/lseek_b"
// CMD: setup="/tmp/lseek_c"  args="past_eof 50 50 /tmp/lseek_c"  cleanup="/tmp/lseek_c"
// CMD:                        args="bad_fd 0 0 /tmp/none"        cleanup=""
// CMD: setup="/tmp/lseek_d"  args="bad_whence 0 0 /tmp/lseek_d"  cleanup="/tmp/lseek_d"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef __wasm__
#include <sys/syscall.h>
static inline long wali_lseek_native(int fd, long offset, int whence) {
    return syscall(SYS_lseek, fd, offset, whence);
}
#define wali_syscall_lseek(fd, offset, whence) wali_lseek_native((fd), (offset), (whence))
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

static long parse_long(const char *s) {
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    long v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return sign * v;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 5) return -1;
    const char *mode = argv[1];
    long offset = parse_long(argv[2]);
    long expected = parse_long(argv[3]);
    const char *path = argv[4];

    int whence;
    int expect_ok = 1;
    if (!strcmp(mode, "set"))             whence = SEEK_SET;
    else if (!strcmp(mode, "end"))        whence = SEEK_END;
    else if (!strcmp(mode, "past_eof"))   whence = SEEK_SET;
    else if (!strcmp(mode, "bad_fd"))     whence = SEEK_SET;
    else if (!strcmp(mode, "bad_whence")) { whence = 9999; expect_ok = 0; }
    else return -1;

    int fd;
    if (!strcmp(mode, "bad_fd")) {
        fd = 99999;
        expect_ok = 0;
    } else {
        fd = wali_syscall_open(path, O_RDONLY, 0);
        if (fd < 0) return -1;
    }

    long r = wali_syscall_lseek(fd, offset, whence);
    if (strcmp(mode, "bad_fd") != 0) wali_syscall_close(fd);

    if (!expect_ok) return (r < 0) ? 0 : -1;
    return (r == expected) ? 0 : -1;
}
