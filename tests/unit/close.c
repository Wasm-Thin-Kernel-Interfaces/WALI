// CMD: setup="/tmp/wali_close_a"  args="ok"        cleanup="/tmp/wali_close_a"
// CMD: setup="/tmp/wali_close_b"  args="double"    cleanup="/tmp/wali_close_b"
// CMD:                             args="negative"  cleanup=""
// CMD:                             args="huge"      cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#define VALID_PATH_A "/tmp/wali_close_a"
#define VALID_PATH_B "/tmp/wali_close_b"

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
    const char *mode = (argc > 1) ? argv[1] : "ok";

    if (!strcmp(mode, "negative")) {
        long r = wali_syscall_close(-1);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "huge")) {
        long r = wali_syscall_close(99999);
        return (r < 0) ? 0 : -1;
    }

    const char *path = !strcmp(mode, "ok") ? VALID_PATH_A : VALID_PATH_B;
    int fd = wali_syscall_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;

    long r1 = wali_syscall_close(fd);
    if (r1 != 0) return -1;

    if (!strcmp(mode, "double")) {
        long r2 = wali_syscall_close(fd);
        return (r2 < 0) ? 0 : -1;
    }
    return 0;
}
