// CMD: setup="/tmp/fdatasync_a" args="ok     /tmp/fdatasync_a"  cleanup="/tmp/fdatasync_a"
// CMD: setup="/tmp/fdatasync_b" args="rdonly /tmp/fdatasync_b"  cleanup="/tmp/fdatasync_b"
// CMD:                           args="bad_fd /tmp/none"          cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_fdatasync") long wali_syscall_fdatasync(int fd);
int wali_fdatasync(int fd) { return (int)wali_syscall_fdatasync(fd); }
#else
#include <sys/syscall.h>
int wali_fdatasync(int fd) { return syscall(SYS_fdatasync, fd); }
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

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode, "bad_fd")) {
        long r = wali_fdatasync(99999);
        return (r < 0) ? 0 : -1;
    }

    int flags = !strcmp(mode, "rdonly") ? O_RDONLY : O_WRONLY;
    int fd = wali_syscall_open(path, flags, 0);
    if (fd < 0) return -1;
    long r = wali_fdatasync(fd);
    wali_syscall_close(fd);
    return (r == 0) ? 0 : -1;
}
