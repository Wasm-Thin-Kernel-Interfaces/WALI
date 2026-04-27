// CMD: setup="create 0644 /tmp/access_fok.txt"      args="F_OK 1 /tmp/access_fok.txt"      cleanup="/tmp/access_fok.txt"
// CMD:                                                args="F_OK 0 /tmp/access_missing.txt"  cleanup=""
// CMD: setup="create 0444 /tmp/access_rok.txt"      args="R_OK 1 /tmp/access_rok.txt"      cleanup="/tmp/access_rok.txt"
// CMD: setup="create 0755 /tmp/access_xok.txt"      args="X_OK 1 /tmp/access_xok.txt"      cleanup="/tmp/access_xok.txt"
// CMD: setup="create 0644 /tmp/access_xfail.txt"    args="X_OK 0 /tmp/access_xfail.txt"    cleanup="/tmp/access_xfail.txt"
// CMD: setup="create 0666 /tmp/access_rwok.txt"     args="RW_OK 1 /tmp/access_rwok.txt"    cleanup="/tmp/access_rwok.txt"
// CMD: setup="dangling_symlink /tmp/access_dangle"  args="F_OK 0 /tmp/access_dangle"       cleanup="/tmp/access_dangle"

#include "wali_start.c"

#include <fcntl.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    const char *op = argv[0];

    if (strcmp(op, "create") == 0) {
        if (argc < 3) return -1;
        long mode = strtol(argv[1], NULL, 8);
        const char *path = argv[2];
        unlink(path);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, (mode_t)mode);
        if (fd < 0) return -1;
        close(fd);
        // open() honors umask; force the exact mode we asked for.
        if (chmod(path, (mode_t)mode) != 0) return -1;
    } else if (strcmp(op, "dangling_symlink") == 0) {
        if (argc < 2) return -1;
        const char *path = argv[1];
        unlink(path);
        if (symlink("/tmp/access_definitely_missing_target", path) != 0) return -1;
    }
    return 0;
}

int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    unlink(argv[0]);
    return 0;
}
#endif

static int parse_mode(const char *s) {
    if (!strcmp(s, "F_OK")) return F_OK;
    if (!strcmp(s, "R_OK")) return R_OK;
    if (!strcmp(s, "W_OK")) return W_OK;
    if (!strcmp(s, "X_OK")) return X_OK;
    if (!strcmp(s, "RW_OK")) return R_OK | W_OK;
    return -1;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 4) return -1;

    int mode = parse_mode(argv[1]);
    if (mode < 0) return -1;
    int expect_ok = (argv[2][0] == '1');
    const char *path = argv[3];

    long r = wali_syscall_access(path, mode);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
