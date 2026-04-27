// CMD: setup="/tmp/fsync_a" args="ok     /tmp/fsync_a"  cleanup="/tmp/fsync_a"
// CMD: setup="/tmp/fsync_b" args="rdonly /tmp/fsync_b"  cleanup="/tmp/fsync_b"
// CMD:                       args="bad_fd /tmp/none"     cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_fsync") long wali_syscall_fsync(int fd);
int wali_fsync(int fd) { return (int)wali_syscall_fsync(fd); }
#else
#include <sys/syscall.h>
int wali_fsync(int fd) { return syscall(SYS_fsync, fd); }
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
        long r = wali_fsync(99999);
        return (r < 0) ? 0 : -1;
    }

    int flags = !strcmp(mode, "rdonly") ? O_RDONLY : O_WRONLY;
    int fd = wali_syscall_open(path, flags, 0);
    if (fd < 0) return -1;
    long r = wali_fsync(fd);
    wali_syscall_close(fd);
    return (r == 0) ? 0 : -1;
}
