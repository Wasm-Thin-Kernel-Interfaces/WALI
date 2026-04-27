// CMD: setup="/tmp/fchmodat_a"  args="0640 /tmp/fchmodat_a"  cleanup="0640 /tmp/fchmodat_a"
// CMD: setup="/tmp/fchmodat_b"  args="0400 /tmp/fchmodat_b"  cleanup="0400 /tmp/fchmodat_b"
// CMD: setup="/tmp/fchmodat_c"  args="0777 /tmp/fchmodat_c"  cleanup="0777 /tmp/fchmodat_c"
// CMD:                           args="0644 /tmp/fchmodat_missing" cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifdef __wasm__
WALI_IMPORT("SYS_fchmodat") long wali_syscall_fchmodat(int dirfd, const char *pathname, int mode, int flags);
int wali_fchmodat(int dirfd, const char *pathname, int mode, int flags) {
    return (int)wali_syscall_fchmodat(dirfd, pathname, mode, flags);
}
#else
#include <sys/syscall.h>
int wali_fchmodat(int dirfd, const char *pathname, int mode, int flags) {
    return syscall(SYS_fchmodat, dirfd, pathname, mode, flags);
}
#endif

#ifdef WALI_TEST_WRAPPER
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc == 0) return 0;
    if (argc == 1) { unlink(argv[0]); return 0; }
    long expected = strtol(argv[0], NULL, 8);
    const char *path = argv[1];
    struct stat st;
    int ok = (stat(path, &st) == 0) && ((st.st_mode & 07777) == (mode_t)expected);
    unlink(path);
    return ok ? 0 : -1;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    int mode = 0;
    for (const char *p = argv[1]; *p; p++) {
        if (*p < '0' || *p > '7') return -1;
        mode = (mode << 3) | (*p - '0');
    }
    const char *path = argv[2];

    long r = wali_fchmodat(AT_FDCWD, path, mode, 0);
    int is_missing = (strstr(path, "missing") != NULL);
    int success = (r == 0);
    int expect_ok = !is_missing;
    return (success == expect_ok) ? 0 : -1;
}
