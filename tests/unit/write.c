// CMD: args="ok"
// CMD: args="zero"
// CMD: args="bad_fd"
// CMD: args="rdonly_fd"
// CMD: args="closed_fd"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#define TMP_PATH "/tmp/wali_write_test"

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    int fd = open(TMP_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    unlink(TMP_PATH);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    if (!strcmp(mode, "bad_fd")) {
        long r = wali_syscall_write(99999, "X", 1);
        return (r < 0) ? 0 : -1;
    }

    int flags = !strcmp(mode, "rdonly_fd") ? O_RDONLY : O_WRONLY;
    int fd = wali_syscall_open(TMP_PATH, flags, 0);
    if (fd < 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "ok")) {
        long r = wali_syscall_write(fd, "HELLO", 5);
        ret = (r == 5) ? 0 : -1;
    } else if (!strcmp(mode, "zero")) {
        long r = wali_syscall_write(fd, "X", 0);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "rdonly_fd")) {
        long r = wali_syscall_write(fd, "X", 1);
        ret = (r < 0) ? 0 : -1;
    } else if (!strcmp(mode, "closed_fd")) {
        wali_syscall_close(fd);
        long r = wali_syscall_write(fd, "X", 1);
        return (r < 0) ? 0 : -1;
    }

    wali_syscall_close(fd);
    return ret;
}
