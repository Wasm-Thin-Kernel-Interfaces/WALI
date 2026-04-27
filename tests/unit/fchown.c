// CMD: setup="/tmp/fchown_a"  args="ok      /tmp/fchown_a   -1 -1"  cleanup="/tmp/fchown_a"
// CMD: setup="/tmp/fchown_b"  args="ok      /tmp/fchown_b   -1  0"  cleanup="/tmp/fchown_b"
// CMD: setup="/tmp/fchown_c"  args="ok      /tmp/fchown_c    0 -1"  cleanup="/tmp/fchown_c"
// CMD:                         args="bad_fd  /tmp/none       -1 -1"  cleanup=""
// CMD:                         args="negative /tmp/none      -1 -1"  cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    unlink(argv[0]);
    return 0;
}
#endif

static int parse_int(const char *s) {
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return sign * v;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 5) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];
    int uid = parse_int(argv[3]);
    int gid = parse_int(argv[4]);

    int fd;
    if (!strcmp(mode, "bad_fd"))        fd = 99999;
    else if (!strcmp(mode, "negative")) fd = -1;
    else if (!strcmp(mode, "ok")) {
        fd = wali_syscall_open(path, O_RDONLY, 0);
        if (fd < 0) return -1;
    } else return -1;

    long r = wali_syscall_fchown(fd, uid, gid);
    if (!strcmp(mode, "ok")) wali_syscall_close(fd);
    return (r == 0) ? 0 : 1;
}
