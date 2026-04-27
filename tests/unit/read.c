// CMD: setup="/tmp/read_a"  args="ok       /tmp/read_a"  cleanup="/tmp/read_a"
// CMD: setup="/tmp/read_b"  args="zero     /tmp/read_b"  cleanup="/tmp/read_b"
// CMD: setup="/tmp/read_c"  args="wronly   /tmp/read_c"  cleanup="/tmp/read_c"
// CMD:                       args="bad_fd   /tmp/none"    cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    write(fd, "WALI_READ_TEST", 14);
    close(fd);
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
        char buf[8];
        long r = wali_syscall_read(99999, buf, 8);
        return (r < 0) ? 0 : -1;
    }

    int flags = !strcmp(mode, "wronly") ? O_WRONLY : O_RDONLY;
    int fd = wali_syscall_open(path, flags, 0);
    if (fd < 0) return -1;

    char buf[32] = {0};
    long r;
    int ret = -1;
    if (!strcmp(mode, "ok")) {
        r = wali_syscall_read(fd, buf, sizeof(buf));
        ret = (r == 14 && memcmp(buf, "WALI_READ_TEST", 14) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "zero")) {
        r = wali_syscall_read(fd, buf, 0);
        ret = (r == 0) ? 0 : -1;
    } else if (!strcmp(mode, "wronly")) {
        // Read on a write-only fd → EBADF.
        r = wali_syscall_read(fd, buf, sizeof(buf));
        ret = (r < 0) ? 0 : -1;
    }
    wali_syscall_close(fd);
    return ret;
}
