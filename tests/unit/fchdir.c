// CMD: args="ok_dir /tmp"
// CMD: args="ok_root /"
// CMD: setup="/tmp/wali_fchdir_file" args="not_dir /tmp/wali_fchdir_file" cleanup="/tmp/wali_fchdir_file"
// CMD: args="bad_fd ."

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_fchdir") long wali_syscall_fchdir(int fd);
int wali_fchdir(int fd) { return (int)wali_syscall_fchdir(fd); }
#else
#include <sys/syscall.h>
int wali_fchdir(int fd) { return syscall(SYS_fchdir, fd); }
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

    int fd;
    int expect_ok = 1;

    if (!strcmp(mode, "bad_fd")) {
        fd = 99999;
        expect_ok = 0;
    } else if (!strcmp(mode, "ok_dir") || !strcmp(mode, "ok_root")) {
        fd = wali_syscall_open(path, O_RDONLY | O_DIRECTORY, 0);
        if (fd < 0) return -1;
    } else if (!strcmp(mode, "not_dir")) {
        // Open a regular file → fchdir on it must fail with ENOTDIR.
        fd = wali_syscall_open(path, O_RDONLY, 0);
        if (fd < 0) return -1;
        expect_ok = 0;
    } else {
        return -1;
    }

    long r = wali_fchdir(fd);
    if (strcmp(mode, "bad_fd") != 0) wali_syscall_close(fd);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
