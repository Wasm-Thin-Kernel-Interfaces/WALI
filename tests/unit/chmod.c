// CMD: setup="0644 /tmp/chmod_a.txt"  args="ok 0777 /tmp/chmod_a.txt"        cleanup="0777 /tmp/chmod_a.txt"
// CMD: setup="0644 /tmp/chmod_b.txt"  args="ok 0600 /tmp/chmod_b.txt"        cleanup="0600 /tmp/chmod_b.txt"
// CMD: setup="0644 /tmp/chmod_c.txt"  args="ok 0000 /tmp/chmod_c.txt"        cleanup="0000 /tmp/chmod_c.txt"
// CMD: setup="0644 /tmp/chmod_d.txt"  args="ok 04755 /tmp/chmod_d.txt"       cleanup="04755 /tmp/chmod_d.txt"
// CMD: setup="0644 /tmp/chmod_e.txt"  args="ok 02755 /tmp/chmod_e.txt"       cleanup="02755 /tmp/chmod_e.txt"
// CMD:                                 args="fail 0777 /tmp/chmod_missing"   cleanup=""

#include "wali_start.c"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 2) return 0;
    long mode = strtol(argv[0], NULL, 8);
    const char *path = argv[1];
    unlink(path);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, (mode_t)mode);
    if (fd < 0) return -1;
    close(fd);
    // open() honors umask; force the requested mode explicitly.
    return chmod(path, (mode_t)mode);
}
int test_cleanup(int argc, char **argv) {
    if (argc == 0) return 0;
    if (argc == 1) {
        unlink(argv[0]);
        return 0;
    }
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
    if (argc < 4) return -1;
    int expect_ok = strcmp(argv[1], "ok") == 0;
    int mode = 0;
    for (const char *p = argv[2]; *p; p++) {
        if (*p < '0' || *p > '7') return -1;
        mode = (mode << 3) | (*p - '0');
    }
    const char *path = argv[3];
    long r = wali_syscall_chmod(path, mode);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
