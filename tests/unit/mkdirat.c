// CMD: setup="parent:/tmp/mkdirat_p" args="ok       /tmp/mkdirat_p subdir" cleanup="/tmp/mkdirat_p/subdir /tmp/mkdirat_p"
// CMD: setup="parent_with_sub:/tmp/mkdirat_q subdir" args="exists /tmp/mkdirat_q subdir" cleanup="/tmp/mkdirat_q/subdir /tmp/mkdirat_q"
// CMD: setup="parent:/tmp/mkdirat_r" args="bad_dirfd /tmp/mkdirat_r foo" cleanup="/tmp/mkdirat_r"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
#include <sys/stat.h>
#include <stdio.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "parent:", 7)) {
        rmdir(argv[0] + 7);
        return mkdir(argv[0] + 7, 0755);
    }
    if (!strncmp(argv[0], "parent_with_sub:", 16)) {
        if (argc < 2) return -1;
        const char *parent = argv[0] + 16;
        const char *sub = argv[1];
        char buf[256];
        snprintf(buf, sizeof(buf), "%s/%s", parent, sub);
        rmdir(buf);
        rmdir(parent);
        if (mkdir(parent, 0755) != 0) return -1;
        return mkdir(buf, 0755);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    // Cleanup args list paths in deepest-first order so rmdir works.
    for (int i = 0; i < argc; i++) rmdir(argv[i]);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 4) return -1;
    const char *mode = argv[1];
    const char *parent = argv[2];
    const char *sub = argv[3];

    int dirfd;
    int expect_ok = 1;
    if (!strcmp(mode, "bad_dirfd")) {
        dirfd = -1;
        expect_ok = 0;
    } else {
        dirfd = wali_syscall_open(parent, O_RDONLY | O_DIRECTORY, 0);
        if (dirfd < 0) return -1;
    }

    if (!strcmp(mode, "exists")) expect_ok = 0;

    long r = wali_syscall_mkdirat(dirfd, sub, 0755);
    if (strcmp(mode, "bad_dirfd") != 0) wali_syscall_close(dirfd);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
