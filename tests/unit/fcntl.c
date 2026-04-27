// CMD: setup="/tmp/fcntl_a"  args="getfl       /tmp/fcntl_a"  cleanup="/tmp/fcntl_a"
// CMD: setup="/tmp/fcntl_b"  args="setfl_append /tmp/fcntl_b" cleanup="/tmp/fcntl_b"
// CMD: setup="/tmp/fcntl_c"  args="getfd       /tmp/fcntl_c"  cleanup="/tmp/fcntl_c"
// CMD: setup="/tmp/fcntl_d"  args="setfd_cloexec /tmp/fcntl_d" cleanup="/tmp/fcntl_d"
// CMD: setup="/tmp/fcntl_e"  args="dupfd       /tmp/fcntl_e"  cleanup="/tmp/fcntl_e"
// CMD: setup="/tmp/fcntl_f"  args="dupfd_cloexec /tmp/fcntl_f" cleanup="/tmp/fcntl_f"
// CMD:                        args="bad_fd      /tmp/none"     cleanup=""

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

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode, "bad_fd")) {
        long r = wali_syscall_fcntl(99999, F_GETFL, 0);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_RDWR, 0);
    if (fd < 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "getfl")) {
        long flags = wali_syscall_fcntl(fd, F_GETFL, 0);
        ret = (flags >= 0 && (flags & O_ACCMODE) == O_RDWR) ? 0 : -1;
    } else if (!strcmp(mode, "setfl_append")) {
        long flags = wali_syscall_fcntl(fd, F_GETFL, 0);
        if (flags < 0) goto out;
        if (wali_syscall_fcntl(fd, F_SETFL, flags | O_APPEND) != 0) goto out;
        long after = wali_syscall_fcntl(fd, F_GETFL, 0);
        ret = (after >= 0 && (after & O_APPEND)) ? 0 : -1;
    } else if (!strcmp(mode, "getfd")) {
        long flags = wali_syscall_fcntl(fd, F_GETFD, 0);
        ret = (flags >= 0 && !(flags & FD_CLOEXEC)) ? 0 : -1;
    } else if (!strcmp(mode, "setfd_cloexec")) {
        if (wali_syscall_fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) goto out;
        long after = wali_syscall_fcntl(fd, F_GETFD, 0);
        ret = (after >= 0 && (after & FD_CLOEXEC)) ? 0 : -1;
    } else if (!strcmp(mode, "dupfd")) {
        long newfd = wali_syscall_fcntl(fd, F_DUPFD, 0);
        if (newfd < 0 || newfd == fd) goto out;
        // F_DUPFD must NOT set CLOEXEC.
        long fdflags = wali_syscall_fcntl((int)newfd, F_GETFD, 0);
        ret = (fdflags >= 0 && !(fdflags & FD_CLOEXEC)) ? 0 : -1;
        wali_syscall_close((int)newfd);
    } else if (!strcmp(mode, "dupfd_cloexec")) {
#ifdef F_DUPFD_CLOEXEC
        long newfd = wali_syscall_fcntl(fd, F_DUPFD_CLOEXEC, 0);
        if (newfd < 0) goto out;
        long fdflags = wali_syscall_fcntl((int)newfd, F_GETFD, 0);
        ret = (fdflags >= 0 && (fdflags & FD_CLOEXEC)) ? 0 : -1;
        wali_syscall_close((int)newfd);
#else
        ret = 0;
#endif
    }

out:
    wali_syscall_close(fd);
    return ret;
}
