// CMD: setup="/tmp/fadvise_a"  args="normal      /tmp/fadvise_a"  cleanup="/tmp/fadvise_a"
// CMD: setup="/tmp/fadvise_b"  args="random      /tmp/fadvise_b"  cleanup="/tmp/fadvise_b"
// CMD: setup="/tmp/fadvise_c"  args="sequential  /tmp/fadvise_c"  cleanup="/tmp/fadvise_c"
// CMD: setup="/tmp/fadvise_d"  args="willneed    /tmp/fadvise_d"  cleanup="/tmp/fadvise_d"
// CMD: setup="/tmp/fadvise_e"  args="dontneed    /tmp/fadvise_e"  cleanup="/tmp/fadvise_e"
// CMD: setup="/tmp/fadvise_f"  args="noreuse     /tmp/fadvise_f"  cleanup="/tmp/fadvise_f"
// CMD:                          args="bad_fd       /tmp/none"        cleanup=""
// CMD: setup="/tmp/fadvise_g"  args="bad_advice  /tmp/fadvise_g"  cleanup="/tmp/fadvise_g"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

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

#ifdef __wasm__
WALI_IMPORT("SYS_fadvise") long wali_syscall_fadvise(int fd, long long offset, long long len, int advice);
int wali_fadvise(int fd, long long offset, long long len, int advice) { return (int)wali_syscall_fadvise(fd, offset, len, advice); }
#else
#include <sys/syscall.h>
int wali_fadvise(int fd, long long offset, long long len, int advice) { return syscall(SYS_fadvise64, fd, offset, len, advice); }
#endif

#ifndef POSIX_FADV_NORMAL
#define POSIX_FADV_NORMAL 0
#define POSIX_FADV_RANDOM 1
#define POSIX_FADV_SEQUENTIAL 2
#define POSIX_FADV_WILLNEED 3
#define POSIX_FADV_DONTNEED 4
#define POSIX_FADV_NOREUSE 5
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    int advice = POSIX_FADV_NORMAL;
    int expect_ok = 1;
    int use_bad_fd = 0;
    if (!strcmp(mode, "normal"))           advice = POSIX_FADV_NORMAL;
    else if (!strcmp(mode, "random"))      advice = POSIX_FADV_RANDOM;
    else if (!strcmp(mode, "sequential"))  advice = POSIX_FADV_SEQUENTIAL;
    else if (!strcmp(mode, "willneed"))    advice = POSIX_FADV_WILLNEED;
    else if (!strcmp(mode, "dontneed"))    advice = POSIX_FADV_DONTNEED;
    else if (!strcmp(mode, "noreuse"))     advice = POSIX_FADV_NOREUSE;
    else if (!strcmp(mode, "bad_fd"))      { use_bad_fd = 1; expect_ok = 0; }
    else if (!strcmp(mode, "bad_advice"))  { advice = 9999; expect_ok = 0; }
    else return -1;

    int fd;
    if (use_bad_fd) {
        fd = 99999;
    } else {
        fd = wali_syscall_open(path, O_RDONLY, 0);
        if (fd < 0) return -1;
    }

    long r = wali_fadvise(fd, 0, 0, advice);
    if (!use_bad_fd) wali_syscall_close(fd);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
