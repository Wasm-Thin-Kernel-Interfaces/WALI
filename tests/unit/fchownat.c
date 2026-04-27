// CMD: setup="/tmp/fchownat_a"  args="ok       /tmp/fchownat_a   -1 -1"  cleanup="/tmp/fchownat_a"
// CMD: setup="/tmp/fchownat_b"  args="ok       /tmp/fchownat_b   -1  0"  cleanup="/tmp/fchownat_b"
// CMD: setup="/tmp/fchownat_c"  args="ok       /tmp/fchownat_c    0 -1"  cleanup="/tmp/fchownat_c"
// CMD:                           args="missing  /tmp/fchownat_miss -1 -1" cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifdef __wasm__
WALI_IMPORT("SYS_fchownat") long wali_syscall_fchownat(int dirfd, const char *pathname, int owner, int group, int flags);
int wali_fchownat(int dirfd, const char *pathname, int owner, int group, int flags) {
    return (int)wali_syscall_fchownat(dirfd, pathname, owner, group, flags);
}
#else
#include <sys/syscall.h>
int wali_fchownat(int dirfd, const char *pathname, int owner, int group, int flags) {
    return syscall(SYS_fchownat, dirfd, pathname, owner, group, flags);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
    const char *path = argv[2];
    int uid = parse_int(argv[3]);
    int gid = parse_int(argv[4]);

    long r = wali_fchownat(AT_FDCWD, path, uid, gid, 0);
    return (r == 0) ? 0 : 1;
}
